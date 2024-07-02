global UI_State *ui_state;

internal void ui_set_state(UI_State *state) {
    ui_state = state;
}

internal UI_State *ui_state_new() {
    Arena *arena = arena_new();
    UI_State *ui = push_array(arena, UI_State, 1);
    ui->arena = arena;
    ui->build_arenas[0] = arena_new();
    ui->build_arenas[1] = arena_new();
    ui->box_table_size = 4096;
    ui->box_table = push_array(arena, UI_Hash_Bucket, ui->box_table_size);
    return ui;
}

internal void ui_pop_event(UI_Event *event) {
    UI_Event *prev = event->prev;
    UI_Event *next = event->next;
    if (prev) {
        prev->next = next;
    } else {
        ui_state->events.first = next;
    }
    if (next) {
        next->prev = prev;
    } else {
        ui_state->events.last = prev;
    }
}

internal bool ui_mouse_hover(UI_Box *box) {
    v2 v = ui_state->mouse_position;
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

internal UI_Event *ui_push_event(UI_Event_Type type) {
    UI_Event *result = push_array(ui_build_arena(), UI_Event, 1);
    result->type = type;
    result->next = nullptr;
    result->prev = nullptr;
    
    UI_Event *last = ui_state->events.last;
    if (last) {
        last->next = result;
        result->prev = last;
    } else {
        ui_state->events.first = result;
    }
    ui_state->events.last = result;
    ui_state->events.count += 1;
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

internal void ui_set_string(UI_Box *box, String8 string) {
    box->string = string;
}

internal void ui_set_focus_active(UI_Hash hash) {
    ui_state->focus_active_id = hash;
}

internal UI_Hash ui_focus_active_id() {
    return ui_state->focus_active_id;
}

internal void ui_set_active(UI_Hash hash) {
    ui_state->active_id = hash;
}

internal UI_Hash ui_active_id() {
    return ui_state->active_id;
}

internal UI_Box *ui_get_root() {
    return ui_state->root;
}

internal UI_Hash ui_hash(String8 text) {
    //@Note djb2 hash algorithm
    UI_Hash hash = 5381;
    int c;
    while (c = *text.data++) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

internal UI_Size ui_size(UI_Size_Type type, f32 value, f32 strictness) {
    UI_Size size = { type, value, strictness };
    return size;
}

internal UI_Box *ui_box_from_hash(UI_Hash hash) {
    UI_Hash_Bucket *hash_bucket = &ui_state->box_table[hash % ui_state->box_table_size];
    for (UI_Box *box = hash_bucket->first; box; box = box->hash_next) {
        if (box->hash == hash) {
            return box;
        }
    }
    return nullptr;
}

internal UI_Box *ui_get_parent() {
    UI_Box *box = nullptr;
    if (!ui_state->parent_stack.is_empty()) {
        box = ui_state->parent_stack.top();
    }
    return box;
}

internal void ui_push_box(UI_Box *box, UI_Box *parent) {
    box->parent = parent;
    if (parent) {
        if (parent->first) {
            box->prev = parent->last;
            parent->last->next = box;
            parent->last = box;
        } else {
            parent->first = box;
            parent->last = box;
        }
    }
}

internal Arena *ui_build_arena() {
    Arena *result = ui_state->build_arenas[ui_state->build_index%ArrayCount(ui_state->build_arenas)];
    return result;
}

internal UI_Box *ui_make_box(UI_Hash hash, UI_Box_Flags flags) {
    UI_Box *parent = ui_get_parent();
    UI_Box *box = ui_box_from_hash(hash);
    bool box_first_frame = box == nullptr;
    if (box_first_frame) {
        box = push_array(ui_state->arena, UI_Box, 1);
        *box = UI_Box();

        UI_Hash_Bucket *hash_bucket = &ui_state->box_table[hash % ui_state->box_table_size];
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

    //@Note Reset per-frame builder options
    box->first = box->last = box->prev = box->next = box->parent = nullptr;
    box->hash = hash;
    box->flags = flags;
    box->pref_size[0] = {};
    box->pref_size[1] = {};
    box->fixed_position = V2();
    box->fixed_size = V2();

    if (parent == nullptr) {
        return box;
    }

    ui_push_box(box, parent);

    assert(!ui_state->font_stack.is_empty());
    box->font_face = ui_state->font_stack.top();

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

internal UI_Box *ui_make_box_from_string(String8 string, UI_Box_Flags flags) {
    UI_Box *box = nullptr;
    UI_Hash hash = ui_hash(string);
    box = ui_make_box(hash, flags);
    return box;
}

internal UI_Signal ui_signal_from_box(UI_Box *box) {
    UI_Signal signal{};
    signal.box = box;
    UI_Box *box_last_frame = nullptr;
    box_last_frame = ui_box_from_hash(box->hash);
    bool box_first_frame = box_last_frame == nullptr;
    if (box_first_frame) {
        return signal;
    }

    signal.key_modifiers = os_event_flags();
    signal.text = str8_zero();
    bool event_in_bounds = ui_mouse_hover(box_last_frame);
    
    for (UI_Event *event = ui_state->events.first, *next = nullptr; event; event = next) {
        next = event->next;

        switch (event->type) {
        case UI_EVENT_MOUSE_PRESS:
        {
            if (event_in_bounds) {
                signal.flags |= UI_SIGNAL_CLICKED;
                // signal.pos = event->pos;
                ui_set_active(box->hash);
                if (box->flags & UI_BOX_KEYBOARD_INPUT) {
                    ui_set_focus_active(box->hash);
                }
            }
            break;
        }
        case UI_EVENT_MOUSE_RELEASE:
        {
            ui_set_active(0);
            break; 
        }
        case UI_EVENT_SCROLL:
        {
            if (event_in_bounds) {
                signal.scroll = event->delta;
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
    }

    if (event_in_bounds) {
        signal.flags |= UI_SIGNAL_HOVER;
    }
    return signal;
}

internal void ui_layout_calc_fixed(UI_Box *box, Axis2 axis) {
    if (!(box->flags & (UI_BOX_FIXED_WIDTH << axis))) {
        f32 size = 0.0f;
        switch (box->pref_size[axis].type) {
        default:
            break;
        case UI_SIZE_PIXELS:
            size = box->pref_size[axis].value;
            break;
        case UI_SIZE_TEXT_CONTENT:
            f32 padding = box->pref_size[axis].value;
            if (axis == AXIS_X) {
                size = get_string_width(box->string, box->font_face);
            } else {
                size = box->font_face->glyph_height;
            }
            size += 2.0f * padding;
            break;
        }
        box->fixed_size[axis] += size;
    }

    for (UI_Box *child = box->first; child != nullptr; child = child->next) {
        ui_layout_calc_fixed(child, axis);
    }
}

internal void ui_layout_calc_upwards_dependent(UI_Box *box, Axis2 axis) {
    if (box->pref_size[axis].type == UI_SIZE_PARENT_PERCENT) {
        for (UI_Box *parent = box->parent; parent != nullptr; parent = parent->parent) {
            bool found = false;
            if (parent->flags & (UI_BOX_FIXED_WIDTH<<axis)) found = true;
            switch (parent->pref_size[axis].type) {
            default:
                break;
            case UI_SIZE_PIXELS:
            case UI_SIZE_TEXT_CONTENT:
                found = true;
                break;
            }

            if (found) {
                box->fixed_size[axis] = box->pref_size[axis].value * parent->fixed_size[axis];
                break; 
            }
        }
    }

    for (UI_Box *child = box->first; child != nullptr; child = child->next) {
        ui_layout_calc_upwards_dependent(child, axis);
    }
}

internal void ui_layout_calc_downwards_dependent(UI_Box *box, Axis2 axis) {
    for (UI_Box *child = box->first; child != nullptr; child = child->next) {
        ui_layout_calc_downwards_dependent(child, axis);
    }

    if (box->pref_size[axis].type == UI_SIZE_CHILDREN_SUM) {
        if (box->child_layout_axis == axis) {
            //@Note Add sizes of children if calculating the layout axis
            for (UI_Box *child = box->first; child != nullptr; child = child->next) {
                box->fixed_size[axis] += child->fixed_size[axis];
            }
        } else {
            //@Note Get max children size if not on layout axis
            f32 max = 0.0f;
            for (UI_Box *child = box->first; child != nullptr; child = child->next) {
                if (child->fixed_size[axis] > max) max = child->fixed_size[axis];
            }
            box->fixed_size[axis] = max;
        }
    }
}

internal void ui_layout_resolve_violations(UI_Box *box, Axis2 axis) {
    
}

internal void ui_layout_place_boxes(UI_Box *box, Axis2 axis) {
    if (box->parent) {
        UI_Box *parent = box->parent;
        //@Note Do not calculate position if box is floating
        if (!(box->flags & (UI_BOX_FLOATING_X << axis))) {
            box->fixed_position[axis] = parent->fixed_position[axis];
            //@Note Add up sibling sizes if on the layout axis, so that we "grow" in the layout
            if (axis == parent->child_layout_axis) {
                for (UI_Box *sibling = parent->first; sibling != box; sibling = sibling->next) {
                    box->fixed_position[axis] += sibling->fixed_size[axis];
                }
            }
        }
    }
    box->rect.p0[axis] = box->fixed_position[axis];
    box->rect.p1[axis] = box->fixed_position[axis] + box->fixed_size[axis];
    
    for (UI_Box *child = box->first; child != nullptr; child = child->next) {
        ui_layout_place_boxes(child, axis);
    }
}

internal void ui_layout_apply(UI_Box *root) {
    for (Axis2 axis = AXIS_X; axis < AXIS_COUNT; axis = (Axis2)(axis + 1)) {
        ui_layout_calc_fixed(root, axis);
        ui_layout_calc_upwards_dependent(root, axis);
        ui_layout_calc_downwards_dependent(root, axis);
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
    local_persist bool first_call = true;
    if (first_call) {
        first_call = false;
    }

    ui_state->animation_dt = animation_dt;

    // printf("UI EVENTS\n");
    UI_Event *ui_event = nullptr;
    for (OS_Event *event = events->first; event; event = event->next) {
        switch (event->type) {
        default:
            assert(0);
            break;
        case OS_EVENT_MOUSEMOVE:
            ui_state->mouse_position.x = (f32)event->pos.x;
            ui_state->mouse_position.y = (f32)event->pos.y;
            break;
        case OS_EVENT_MOUSEDOWN:
            ui_event = ui_push_event(UI_EVENT_MOUSE_PRESS);
            ui_event->pos = event->pos;
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
            // printf("insert text: %.*s\n", (int)ui_event->text.count, ui_event->text.data);
            break;
        }
    }

    ui_push_font_face(default_font_face);

    // printf("BUILD ROOT\n");
    UI_Box *root = ui_make_box_from_string(str8_lit("~Root"), UI_BOX_NIL);
    root->flags = UI_BOX_FLOATING_X | UI_BOX_FLOATING_Y | UI_BOX_FIXED_WIDTH | UI_BOX_FIXED_HEIGHT;
    root->fixed_position = V2();
    root->fixed_size = os_get_window_dim(window_handle);
    ui_state->root = root;
    ui_state->parent_stack.push(root);

    ui_state->mouse_captured = false;
    ui_state->keyboard_captured = false;
    // printf("START BUILDING\n");
}

internal void ui_end_build() {
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
