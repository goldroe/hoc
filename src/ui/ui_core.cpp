global UI_State *ui_state;

internal UI_Box *ui_get_root() {
    return ui_state->root;
}

internal f32 ui_animation_dt() {
    return ui_state->animation_dt;
}

internal void ui_set_state(UI_State *state) {
    ui_state = state;
}

internal UI_State *ui_state_new() {
    Arena *arena = make_arena(get_virtual_allocator());
    UI_State *ui = push_array(arena, UI_State, 1);
    ui->arena = arena;
    ui->build_arenas[0] = make_arena(get_virtual_allocator());
    ui->build_arenas[1] = make_arena(get_virtual_allocator());
    ui->box_table_size = 4096;
    ui->box_table = push_array(arena, UI_Hash_Bucket, ui->box_table_size);
    return ui;
}

internal UI_Event *ui_push_event(UI_Event_Type type) {
    UI_Event *result = push_array(ui_build_arena(), UI_Event, 1);
    result->type = type;
    result->next = nullptr;
    result->prev = nullptr;

    DLLPushBack(ui_state->events.first, ui_state->events.last, result, next, prev);
    ui_state->events.count += 1;
    return result;
}

internal void ui_pop_event(UI_Event *event) {
    DLLRemove(ui_state->events.first, ui_state->events.last, event, next, prev);
    ui_state->events.count -= 1;
}

internal v2 ui_get_mouse() {
    v2 result = ui_state->mouse_position;
    return result;
}

internal bool ui_mouse_hover(UI_Box *box) {
    v2 v = ui_get_mouse();
    bool result = v.x >= box->rect.x0 &&
        v.x <= box->rect.x1 &&
        v.y >= box->rect.y0 &&
        v.y <= box->rect.y1;
    return result;
}

internal bool ui_key_press(OS_Key key) {
    bool result = false;
    for (UI_Event *event = ui_state->events.first; event; event = event->next) {
        if (event->type == UI_EVENT_PRESS) {
            if (event->key == key) {
                ui_pop_event(event);
                result = true;
                break;
            }
        }
    }
    return result;
}

internal bool ui_key_release(OS_Key key) {
    bool result = false;
    for (UI_Event *event = ui_state->events.first; event; event = event->next) {
        if (event->type == UI_EVENT_RELEASE) {
            if (event->key == key) {
                ui_pop_event(event);
                result = true;
                break;
            }
        }
    }
    return result;
}

internal v2 measure_string_size(String8 string, Face *font_face) {
    v2 result = V2();
    for (u64 i = 0; i < string.count; i++) {
        u8 c = string.data[i];
        if (c == '\n') {
            result.x = 0.f;
            result.y += font_face->glyph_height;
            continue;
        }
        Glyph g = font_face->glyphs[c];
        result.x += g.ax;
    }
    return result;
}

internal f32 get_string_width(String8 text, Face *font_face) {
    f32 result = 0.0f;
    for (u64 i = 0; i < text.count; i++) {
        u8 c = text.data[i];
        Glyph g = font_face->glyphs[c];
        result += g.ax;
    }
    return result;
}

internal void ui_set_custom_draw(UI_Box *box, UI_Box_Draw_Proc *draw_proc, void *user_data) {
    box->custom_draw_proc = draw_proc;
    box->draw_data = user_data;
}

internal void ui_box_set_string(UI_Box *box, String8 string) {
    String8 display_string = string;
    u64 hash_start = str8_find_substr(string, str8_lit("###"));
    if (hash_start < string.count) {
        display_string.count -= hash_start;
    }
    box->string = str8_copy(ui_build_arena(), display_string);
}

//@Note Active setters/getters
internal void ui_set_focus_active(UI_Key key) {
    ui_state->focus_active_id = key;
}

internal UI_Key ui_focus_active_id() {
    return ui_state->focus_active_id;
}

internal void ui_set_active(UI_Key key) {
    ui_state->active_id = key;
}

internal UI_Key ui_active_id() {
    return ui_state->active_id;
}

internal UI_Size ui_size(UI_Size_Type type, f32 value, f32 strictness) {
    UI_Size size = { type, value, strictness };
    return size;
}

internal UI_Box *ui_box_from_key(UI_Key key) {
    UI_Hash_Bucket *hash_bucket = &ui_state->box_table[key % ui_state->box_table_size];
    for (UI_Box *box = hash_bucket->first; box; box = box->hash_next) {
        if (box->key == key) {
            return box;
        }
    }
    return nullptr;
}

internal void ui_push_box(UI_Box *box, UI_Box *parent) {
    box->parent = parent;
    if (parent) {
        DLLPushBack(parent->first, parent->last, box, next, prev);
        parent->child_count += 1;
    }
}

internal Arena *ui_build_arena() {
    Arena *result = ui_state->build_arenas[ui_state->build_index%ArrayCount(ui_state->build_arenas)];
    return result;
}

internal UI_Box *ui_make_box(UI_Key key, UI_Box_Flags flags) {
    UI_Box *parent = ui_top_parent();
    UI_Box *box = ui_box_from_key(key);
    bool box_first_frame = box == nullptr;
    if (box_first_frame) {
        box = push_array(ui_state->arena, UI_Box, 1);
        *box = UI_Box();

        UI_Hash_Bucket *hash_bucket = &ui_state->box_table[key % ui_state->box_table_size];
        if (!hash_bucket->first) {
            hash_bucket->first = box;
            hash_bucket->last = box;
        } else {
            UI_Box *last = hash_bucket->last;
            last->hash_next = box;
            box->hash_prev = last;
            hash_bucket->last = box;
        }
    }
    box->box_id = ui_state->build_counter;
    ui_state->build_counter += 1;

    //@Note Reset per-frame builder options
    box->first = box->last = box->prev = box->next = box->parent = nullptr;
    box->child_count = 0;
    box->key = key;
    box->flags = flags;
    box->pref_size[0] = {};
    box->pref_size[1] = {};
    box->fixed_position = V2();
    box->fixed_size = V2();

    if (parent == nullptr) {
        return box;
    }

    ui_push_box(box, parent);
    
    if (!ui_state->font_stack.is_empty()) {
        box->font_face = ui_state->font_stack.top();
    }

    if (!ui_state->fixed_x_stack.is_empty()) {
        box->flags |= UI_BOX_FLOATING_X;
        box->fixed_position[AXIS_X] = ui_state->fixed_x_stack.top();
    }
    if (!ui_state->fixed_y_stack.is_empty()) {
        box->flags |= UI_BOX_FLOATING_Y;
        box->fixed_position[AXIS_Y] = ui_state->fixed_y_stack.top();
    }

    if (!ui_state->fixed_width_stack.is_empty()) {
        box->flags |= UI_BOX_FIXED_WIDTH;
        box->fixed_size[AXIS_X] = ui_state->fixed_width_stack.top();
    } else {
        box->pref_size[AXIS_X] = ui_state->pref_width_stack.top();
    }
    if (!ui_state->fixed_height_stack.is_empty()) {
        box->flags |= UI_BOX_FIXED_HEIGHT;
        box->fixed_size[AXIS_Y] = ui_state->fixed_height_stack.top();
    }
    else {
        box->pref_size[AXIS_Y] = ui_state->pref_height_stack.top();
    }

    if (!ui_state->text_alignment_stack.is_empty()) {
        box->text_alignment = ui_state->text_alignment_stack.top();
    }
    if (!ui_state->child_layout_axis_stack.is_empty()) {
        box->child_layout_axis = ui_state->child_layout_axis_stack.top();
    }

    if (box->flags & UI_BOX_DRAW_BACKGROUND) {
        box->background_color = ui_state->background_color_stack.top();
    }
    if (box->flags & UI_BOX_DRAW_BORDER) {
        box->border_color = ui_state->border_color_stack.top();
    }
    if (box->flags & UI_BOX_DRAW_TEXT) {
        box->text_color = ui_state->text_color_stack.top();
    }

    f32 fast_rate = 10.f * ui_state->animation_dt;
    box->view_offset.x += fast_rate * (box->view_offset_target.x - box->view_offset.x);
    box->view_offset.y += fast_rate * (box->view_offset_target.y - box->view_offset.y);
    
    //@Note Auto pop UI stacks
    {
        if (ui_state->font_stack.auto_pop) { ui_state->font_stack.pop(); ui_state->font_stack.auto_pop = false; }
        if (ui_state->parent_stack.auto_pop) { ui_state->parent_stack.pop(); ui_state->parent_stack.auto_pop = false; }
        if (ui_state->fixed_x_stack.auto_pop) { ui_state->fixed_x_stack.pop(); ui_state->fixed_x_stack.auto_pop = false; }
        if (ui_state->fixed_y_stack.auto_pop) { ui_state->fixed_y_stack.pop(); ui_state->fixed_y_stack.auto_pop = false; }
        if (ui_state->fixed_width_stack.auto_pop) { ui_state->fixed_width_stack.pop(); ui_state->fixed_width_stack.auto_pop = false; }
        if (ui_state->fixed_height_stack.auto_pop) { ui_state->fixed_height_stack.pop(); ui_state->fixed_height_stack.auto_pop = false; }
        if (ui_state->pref_width_stack.auto_pop) { ui_state->pref_width_stack.pop(); ui_state->pref_width_stack.auto_pop = false; }
        if (ui_state->pref_height_stack.auto_pop) { ui_state->pref_height_stack.pop(); ui_state->pref_height_stack.auto_pop = false; }
        if (ui_state->child_layout_axis_stack.auto_pop) { ui_state->child_layout_axis_stack.pop(); ui_state->child_layout_axis_stack.auto_pop = false; }
        if (ui_state->text_alignment_stack.auto_pop) { ui_state->text_alignment_stack.pop(); ui_state->text_alignment_stack.auto_pop = false; }
        if (ui_state->background_color_stack.auto_pop) { ui_state->background_color_stack.pop(); ui_state->background_color_stack.auto_pop = false; }
        if (ui_state->border_color_stack.auto_pop) { ui_state->border_color_stack.pop(); ui_state->border_color_stack.auto_pop = false; }
        if (ui_state->text_color_stack.auto_pop) { ui_state->text_color_stack.pop(); ui_state->text_color_stack.auto_pop = false; }
    }
    return box;
}

//@Note Key-ing functions

//@Note djb2 hash algorithm
internal u64 ui_hash_from_string(u64 seed, String8 string) {
    u64 result = seed;
    for (u64 i = 0; i < string.count; i++) {
        result = ((result << 5) + result) + string.data[i];
    }
    return result;
}

internal String8 hash_part_from_string(String8 string) {
    String8 hash_part = string;
    u64 hash_start = str8_find_substr(string, str8_lit("###"));
    if (hash_start < string.count) {
        hash_part = str8_jump(string, hash_start);
    }
    return hash_part;     
}

internal UI_Key ui_key_from_string(UI_Key seed_key, String8 string) {
    String8 hash_part = hash_part_from_string(string);
    UI_Key key = (UI_Key)ui_hash_from_string((u64)seed_key, hash_part);
    return key;
}

internal UI_Key ui_seed_key(UI_Box *parent) {
    UI_Key seed_key = 0;
    UI_Box *inherited_seed_parent = parent;
    while (inherited_seed_parent) {
        if (inherited_seed_parent->key != 0) {
            seed_key = inherited_seed_parent->key;
            break;
        }
        inherited_seed_parent = inherited_seed_parent->parent;
    }
    return seed_key;
}

internal UI_Box *ui_make_box_from_string(UI_Box_Flags flags, String8 string) {
    UI_Box *box = nullptr;
    UI_Box *parent = ui_top_parent();

    UI_Key seed_key = ui_seed_key(parent);
    UI_Key key = ui_key_from_string(seed_key, string);
    box = ui_make_box(key, flags);

    if (box->flags & UI_BOX_DRAW_TEXT) {
        ui_box_set_string(box, string);
    }

    return box;
}

internal UI_Box *ui_make_box_from_stringf(UI_Box_Flags flags, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(ui_build_arena(), fmt, args);
    va_end(args);
    UI_Box *box = ui_make_box_from_string(flags, string);
    return box;
}

internal UI_Signal ui_signal_from_box(UI_Box *box) {
    UI_Signal signal{};
    signal.box = box;
    signal.key_modifiers = os_event_flags();

    UI_Box *box_last_frame = ui_box_from_key(box->key);
    bool box_first_frame = box_last_frame == nullptr;
    if (box_first_frame) {
        return signal;
    }

    bool event_in_bounds = ui_mouse_hover(box);

    bool do_later = false;
    for (int i = 0; i < ui_state->last_build_collection.count; i++) {
        UI_Box *collect_box = &ui_state->last_build_collection.data[i];
        if (collect_box->box_id > box->box_id && ui_mouse_hover(collect_box)) {
            do_later = true;
            break;
        }
    }

    //@Note Consume UI events for box
    for (UI_Event *event = ui_state->events.first, *next = nullptr; event; event = next) {
        next = event->next;
        bool taken = false;

        switch (event->type) {
        case UI_EVENT_MOUSE_PRESS:
        {
            if (event_in_bounds && !do_later) {
                signal.flags |= UI_SIGNAL_CLICKED;
                // signal.pos = event->pos;
                ui_set_active(box->key);
                if (box->flags & UI_BOX_KEYBOARD_INPUT) {
                    ui_set_focus_active(box->key);
                }
                taken = true;
            }
            break;
        }
        case UI_EVENT_MOUSE_RELEASE:
        {
            if (event_in_bounds && !do_later) {
                // ui_set_active(0);
                signal.flags |= UI_SIGNAL_RELEASED;
            }
            break; 
        }
        case UI_EVENT_SCROLL:
        {
            if (event_in_bounds && !do_later) {
                taken = true;
                signal.scroll = event->delta;
                signal.flags |= UI_SIGNAL_SCROLL;
            }
            break;
        }
        case UI_EVENT_PRESS:
        {
            signal.flags |= UI_SIGNAL_PRESSED;
            signal.key = event->key;
            // signal.key_modifiers = event->key_modifiers;
            break;
        }
        case UI_EVENT_RELEASE:
        {
            signal.flags |= UI_SIGNAL_RELEASED;
            break; 
        }
        case UI_EVENT_TEXT:
        {
            signal.text = str8_concat(ui_build_arena(), signal.text, event->text);
            break;
        }
        }

        if (taken) {
            ui_pop_event(event);
        }
    }

    if (event_in_bounds) {
        signal.flags |= UI_SIGNAL_HOVER;
    }

    return signal;
}

internal void ui_layout_calc_fixed(UI_Box *root, Axis2 axis) {
    if (!(root->flags & (UI_BOX_FIXED_WIDTH << axis))) {
        f32 size = 0.0f;
        switch (root->pref_size[axis].type) {
        default:
            break;
        case UI_SIZE_PIXELS:
            size = root->pref_size[axis].value;
            break;
        case UI_SIZE_TEXT_CONTENT:
            f32 padding = root->pref_size[axis].value;
            if (axis == AXIS_X) {
                size = get_string_width(root->string, root->font_face);
            } else {
                size = root->font_face->glyph_height;
            }
            size += 2.0f * padding;
            break;
        }
        root->fixed_size[axis] += size;
    }

    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_calc_fixed(child, axis);
    }
}

internal void ui_layout_calc_upwards_dependent(UI_Box *root, Axis2 axis) {
    if (root->pref_size[axis].type == UI_SIZE_PARENT_PERCENT) {
        for (UI_Box *parent = root->parent; parent != nullptr; parent = parent->parent) {
            bool found = false;
            if (parent->flags & (UI_BOX_FIXED_WIDTH<<axis)) found = true;
            switch (parent->pref_size[axis].type) {
            case UI_SIZE_PIXELS:
            case UI_SIZE_TEXT_CONTENT:
                found = true;
                break;
            }

            if (found) {
                root->fixed_size[axis] = root->pref_size[axis].value * parent->fixed_size[axis];
                break; 
            }
        }
    }

    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_calc_upwards_dependent(child, axis);
    }
}

internal void ui_layout_calc_downwards_dependent(UI_Box *root, Axis2 axis) {
    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_calc_downwards_dependent(child, axis);
    }

    if (root->pref_size[axis].type == UI_SIZE_CHILDREN_SUM) {
        if (root->child_layout_axis == axis) {
            //@Note Add sizes of children if on the layout axis
            for (UI_Box *child = root->first; child != nullptr; child = child->next) {
                root->fixed_size[axis] += child->fixed_size[axis];
            }
        } else {
            //@Note Get max children size if not on layout axis
            f32 max = 0.0f;
            for (UI_Box *child = root->first; child != nullptr; child = child->next) {
                if (child->fixed_size[axis] > max) max = child->fixed_size[axis];
            }
            root->fixed_size[axis] = max;
        }
    }
}

internal void ui_layout_resolve_violations(UI_Box *root, Axis2 axis) {
    //@Note Resolve by basic clipping if not on the layout axis
    if (axis != root->child_layout_axis && !(root->flags & (UI_BOX_OVERFLOW_X << axis))) {
        f32 max_size = root->fixed_size[axis];
        for (UI_Box *child = root->first; child != nullptr; child = child->next) {
            f32 child_size = child->fixed_size[axis];
            f32 violation = child_size - max_size;
            if (violation > 0) {
                child_size -= violation;
            }
            child->fixed_size[axis] = child_size;
        }
    }

    //@Note Resolve by doing percent fixup if on the layout axis
    if (axis == root->child_layout_axis && !(root->flags & (UI_BOX_OVERFLOW_X << axis))) {
        f32 max_size = root->fixed_size[axis];
        f32 total_size = 0.f;
        f32 total_weighted_size = 0.f;
        for (UI_Box *child = root->first; child != nullptr; child = child->next) {
            if (!(child->flags & UI_BOX_FLOATING_X<<axis)) {
                total_size += child->fixed_size[axis];
                total_weighted_size += child->fixed_size[axis] * (1 - child->pref_size[axis].strictness);
            }
        }

        f32 violation = total_size - max_size;
        if (violation > 0) {
            for (UI_Box *child = root->first; child != nullptr; child = child->next) {
                if (!(child->flags & UI_BOX_FLOATING_X<<axis)) {
                    f32 fixup = child->fixed_size[axis] * (1 - child->pref_size[axis].strictness);
                    fixup = ClampBot(fixup, 0);
                    f32 fix_pct = violation / total_weighted_size;
                    fix_pct = Clamp(fix_pct, 0, 1);
                    child->fixed_size[axis] -= fixup * fix_pct;
                }
            }
        }
    }

    //@Note Fixup upwards-relative sizes
    if (root->flags & (UI_BOX_OVERFLOW_X << axis)) {
        for (UI_Box *child = root->first; child != nullptr; child = child->next) {
            if (child->pref_size[axis].type == UI_SIZE_PARENT_PERCENT) {
                child->fixed_size[axis] = root->fixed_size[axis] * child->pref_size[axis].value;
            }
        }
    }
    
    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_resolve_violations(child, axis);
    }
}

internal void ui_layout_place_boxes(UI_Box *root, Axis2 axis) {
    f32 p = 0.f;
    UI_Box *parent = root->parent;
    if (parent) {
        p = parent->fixed_position[axis];
        if (!(root->flags & (UI_BOX_FLOATING_X << axis))) {
            //@Note Add up sibling sizes if on the layout axis, so that we "grow" in the layout
            if (axis == parent->child_layout_axis) {
                for (UI_Box *sibling = parent->first; sibling != root; sibling = sibling->next) {
                    p += sibling->fixed_size[axis];
                }
            }
        }
    }
    p += root->fixed_position[axis];

    root->rect.p0[axis] = p;
    root->rect.p1[axis] = p + root->fixed_size[axis];
    
    for (UI_Box *child = root->first; child != nullptr; child = child->next) {
        ui_layout_place_boxes(child, axis);
    }
}

internal void ui_layout_apply(UI_Box *root) {
    for (Axis2 axis = AXIS_X; axis < AXIS_COUNT; axis = (Axis2)(axis + 1)) {
        ui_layout_calc_fixed(root, axis);
        ui_layout_calc_upwards_dependent(root, axis);
        ui_layout_calc_downwards_dependent(root, axis);
        ui_layout_resolve_violations(root, axis);
        ui_layout_place_boxes(root, axis);
    }
}

internal v2 ui_get_text_position(UI_Box *box) {
    // v2 text_position = V2(0.f, box->rect.y0 + .25f * rect_height(box->rect));
    v2 text_position = V2(0.f, box->rect.y0);
    switch (box->text_alignment) {
    case UI_TEXT_ALIGN_CENTER:
        text_position.x = box->rect.x0 + 0.5f * rect_width(box->rect);
        text_position.x -= 0.5f * get_string_width(box->string, box->font_face);
        text_position.x = Max(text_position.x, box->rect.x0);
        break;
    case UI_TEXT_ALIGN_LEFT:
        text_position.x = box->rect.x0;
        break;
    case UI_TEXT_ALIGN_RIGHT:
        text_position.x = box->rect.x1 - get_string_width(box->string, box->font_face);
        text_position.x = Max(text_position.x, box->rect.x0);
        break;
    }
    return text_position;
}

internal void ui_begin_build(f32 animation_dt, OS_Handle window_handle, OS_Event_List *events) {
    ui_state->animation_dt = animation_dt;

    UI_Event *ui_event = nullptr;
    for (OS_Event *event = events->first; event; event = event->next) {
        switch (event->type) {
        default:
            Assert(0);
            break;
        case OS_EVENT_MOUSEMOVE:
            ui_state->mouse_position.x = (f32)event->pos.x;
            ui_state->mouse_position.y = (f32)event->pos.y;
            break;
        case OS_EVENT_MOUSEDOWN:
            ui_event = ui_push_event(UI_EVENT_MOUSE_PRESS);
            ui_event->pos = event->pos;
            ui_event->key = event->key;
            break;
        case OS_EVENT_MOUSEUP:
            ui_event = ui_push_event(UI_EVENT_MOUSE_RELEASE);
            ui_event->pos = event->pos;
            break;
        case OS_EVENT_PRESS:
            ui_event = ui_push_event(UI_EVENT_PRESS);
            ui_event->key = event->key;
            break;
        case OS_EVENT_RELEASE:
            ui_event = ui_push_event(UI_EVENT_RELEASE);
            ui_event->key = event->key;
            break;
        case OS_EVENT_SCROLL:
            ui_event = ui_push_event(UI_EVENT_SCROLL);
            ui_event->delta = event->delta;
            break;
        case OS_EVENT_TEXT:
            ui_event = ui_push_event(UI_EVENT_TEXT);
            ui_event->text = str8_copy(ui_build_arena(), event->text);
            break;
        }
    }

    ui_push_font_face(default_fonts[FONT_DEFAULT]);

    ui_state->mouse_captured = false;
    ui_state->keyboard_captured = false;
    ui_state->build_counter = 0;

    UI_Box *root = ui_make_box_from_string(UI_BOX_NIL, str8_lit("~Root"));
    root->flags = UI_BOX_FLOATING_X | UI_BOX_FLOATING_Y | UI_BOX_FIXED_WIDTH | UI_BOX_FIXED_HEIGHT;
    root->fixed_position = V2();
    root->fixed_size = os_get_window_dim(window_handle);
    ui_state->root = root;
    ui_state->parent_stack.push(root);
}

//@Note Get rect, hash and id of each box for collections
// Used for checking overlapping boxes
internal void ui_collect_build_boxes(UI_Box *root) {
    for (UI_Box *child = root->first; child; child = child->next) {
        ui_collect_build_boxes(child);
    }
    UI_Box box{};
    box.key = root->key;
    box.box_id = root->box_id;
    box.rect = root->rect;
    ui_state->last_build_collection.push(box);
}

internal void ui_end_build() {
    ui_state->last_build_collection.reset_count();
    ui_collect_build_boxes(ui_state->root);

    ui_state->build_index += 1;

    ui_state->events.first = nullptr;
    ui_state->events.last = nullptr;

    //@Note Clear build for next frame
    arena_clear(ui_build_arena());

    //@Note Pop builder stacks 
    ui_state->font_stack.reset_count();
    ui_state->parent_stack.reset_count();
    ui_state->fixed_x_stack.reset_count();
    ui_state->fixed_y_stack.reset_count();
    ui_state->fixed_width_stack.reset_count();
    ui_state->fixed_height_stack.reset_count();
    ui_state->pref_width_stack.reset_count();
    ui_state->pref_height_stack.reset_count();
    ui_state->text_alignment_stack.reset_count();
    ui_state->child_layout_axis_stack.reset_count();
    ui_state->background_color_stack.reset_count();
    ui_state->border_color_stack.reset_count();
    ui_state->text_color_stack.reset_count();
}

#define ui_stack_set_next(Stack, V) ui_state->Stack##_stack.push(V); \
    ui_state->Stack##_stack.auto_pop = true;    \

internal UI_Box *ui_top_parent() { UI_Box *result = ui_state->parent_stack.is_empty() ? NULL : ui_state->parent_stack.top(); return result; }

internal void ui_set_next_font_face(Face *v) { ui_stack_set_next(font, v); }
internal void ui_set_next_parent(UI_Box *v) { ui_stack_set_next(parent, v) }
internal void ui_set_next_fixed_x(f32 v) { ui_stack_set_next(fixed_x, v) }
internal void ui_set_next_fixed_y(f32 v) { ui_stack_set_next(fixed_y, v) }
internal void ui_set_next_fixed_width(f32 v) { ui_stack_set_next(fixed_width, v) }
internal void ui_set_next_fixed_height(f32 v) { ui_stack_set_next(fixed_height, v) }
internal void ui_set_next_pref_width(UI_Size v) { ui_stack_set_next(pref_width, v) }
internal void ui_set_next_pref_height(UI_Size v) { ui_stack_set_next(pref_height, v) }
internal void ui_set_next_child_layout(Axis2 v) { ui_stack_set_next(child_layout_axis, v) }
internal void ui_set_next_text_alignment(UI_Text_Align v) { ui_stack_set_next(text_alignment, v) }
internal void ui_set_next_background_color(v4 v) { ui_stack_set_next(background_color, v) }
internal void ui_set_next_border_color(v4 v) { ui_stack_set_next(border_color, v) }
internal void ui_set_next_text_color(v4 v) { ui_stack_set_next(text_color, v) }

internal void ui_push_font_face(Face *font_face) { ui_state->font_stack.push(font_face); }
internal void ui_push_fixed_x(f32 x) { ui_state->fixed_x_stack.push(x); }
internal void ui_push_fixed_y(f32 y) { ui_state->fixed_y_stack.push(y); }
internal void ui_push_fixed_width(f32 width) { ui_state->fixed_width_stack.push(width); }
internal void ui_push_fixed_height(f32 height) { ui_state->fixed_height_stack.push(height); }
internal void ui_push_parent(UI_Box *parent) { ui_state->parent_stack.push(parent); }
internal void ui_push_child_layout_axis(Axis2 axis) { ui_state->child_layout_axis_stack.push(axis); }
internal void ui_push_pref_width(UI_Size width) { ui_state->pref_width_stack.push(width); }
internal void ui_push_pref_height(UI_Size height) { ui_state->pref_height_stack.push(height); }
internal void ui_push_background_color(v4 color) { ui_state->background_color_stack.push(color); }
internal void ui_push_border_color(v4 color) { ui_state->border_color_stack.push(color); }
internal void ui_push_text_color(v4 color) { ui_state->text_color_stack.push(color); }
internal void ui_push_text_alignment(UI_Text_Align alignment) { ui_state->text_alignment_stack.push(alignment); }

internal void ui_pop_font_face() { ui_state->font_stack.pop(); }
internal void ui_pop_fixed_x() { ui_state->fixed_x_stack.pop(); }
internal void ui_pop_fixed_y() { ui_state->fixed_y_stack.pop(); }
internal void ui_pop_fixed_width() { ui_state->fixed_width_stack.pop(); }
internal void ui_pop_fixed_height() { ui_state->fixed_height_stack.pop(); }
internal void ui_pop_parent() { ui_state->parent_stack.pop(); }
internal void ui_pop_child_layout_axis() { ui_state->child_layout_axis_stack.pop(); }
internal void ui_pop_pref_height() { ui_state->pref_height_stack.pop(); }
internal void ui_pop_pref_width() { ui_state->pref_width_stack.pop(); }
internal void ui_pop_background_color() { ui_state->background_color_stack.pop(); }
internal void ui_pop_border_color() { ui_state->border_color_stack.pop(); }
internal void ui_pop_text_color() { ui_state->text_color_stack.pop(); }
internal void ui_pop_text_alignment() { ui_state->text_alignment_stack.pop(); }
