internal UI_Signal ui_label(String8 string) {
    UI_Box *box = ui_make_box_from_string(UI_BOX_DRAW_TEXT, string);
    return ui_signal_from_box(box);
}

internal UI_Signal ui_labelf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(ui_build_arena(), fmt, args);
    va_end(args);
    return ui_label(string);
}

internal UI_Signal ui_button(String8 string) {
    UI_Box *box = ui_make_box_from_string(UI_BOX_CLICKABLE | UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_TEXT | UI_BOX_DRAW_HOT_EFFECTS | UI_BOX_DRAW_ACTIVE_EFFECTS, string);
    return ui_signal_from_box(box);
}

internal UI_Signal ui_buttonf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    String8 string = str8_pushfv(ui_build_arena(), fmt, args);
    va_end(args);
    return ui_button(string);
}

struct UI_Line_Edit_Draw_Data {
    String8 edit_string;
    s64 cursor;
};

internal UI_BOX_CUSTOM_DRAW_PROC(ui_draw_line_edit) {
    UI_Line_Edit_Draw_Data *draw_data = (UI_Line_Edit_Draw_Data *)user_data;
    String8 edit_string = draw_data->edit_string;
    box->string = edit_string;
    v2 text_position = ui_text_position(box);
    text_position += box->view_offset;
    draw_string(edit_string, box->font, box->text_color, text_position);

    String8 string_before_cursor = edit_string;
    string_before_cursor.count = (u64)draw_data->cursor;
    v2 c_pos = text_position + measure_string_size(string_before_cursor, box->font);
    Rect c_rect = make_rect(c_pos.x, c_pos.y, 2.f, box->font->glyph_height);
    if (ui_key_match(box->key, ui_state->focus_active_box_key)) {
        draw_rect(c_rect, box->text_color);
    }
}

internal UI_Signal ui_line_edit(String8 name, void *buffer, u64 max_buffer_capacity, u64 *buffer_pos,  u64 *buffer_count) {
    ui_set_next_hover_cursor(OS_CURSOR_IBEAM);
    UI_Box *box = ui_make_box_from_string(UI_BOX_CLICKABLE | UI_BOX_KEYBOARD_CLICKABLE |
         UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_BORDER | UI_BOX_DRAW_HOT_EFFECTS | UI_BOX_DRAW_ACTIVE_EFFECTS, name);
    UI_Signal signal = ui_signal_from_box(box);

    u64 pos = *buffer_pos;
    u64 count = *buffer_count;

    if (signal.text.data) {
        u64 text_count = Min(signal.text.count, max_buffer_capacity - count);
        if (pos == text_count) {
            MemoryCopy((u8*)buffer + pos, signal.text.data, text_count);
        } else {
            MemoryCopy((u8*)buffer + pos + text_count, (u8*)buffer + pos, text_count);
            MemoryCopy((u8*)buffer + pos, signal.text.data, text_count);
        }
        pos += text_count;
        count += text_count;
    }

    if (ui_pressed(signal)) {
        switch (signal.key) {
        case OS_KEY_BACKSPACE:
            if (pos > 0) {
                MemoryCopy((u8*)buffer + pos - 1, (u8*)buffer + pos, count - pos);
                pos -= 1;
                count -= 1;
            }
            break;
        case OS_KEY_LEFT:
            if (pos > 0) {
                pos -= 1;
            }
            break; 
        case OS_KEY_RIGHT:
            if (pos < count) {
                pos += 1;
            }
            break;
        }
    }

    *buffer_count = count;
    *buffer_pos = pos;

    UI_Line_Edit_Draw_Data *draw_data = push_array(ui_build_arena(), UI_Line_Edit_Draw_Data, 1);
    draw_data->edit_string = str8((u8 *)buffer, *buffer_count);
    draw_data->cursor = *buffer_pos;
    ui_set_custom_draw(box, ui_draw_line_edit, draw_data);
    return signal;
}

internal UI_Scroll_Pt ui_scroll_bar(String8 name, Axis2 axis, UI_Size flip_axis_size, UI_Scroll_Pt scroll_pt, Rng_S64 view_rng, s64 view_indices) {
    UI_Scroll_Pt new_pt = scroll_pt;
    Axis2 flip_axis = axis_flip(axis);

    s64 scroll_indices = view_indices + rng_s64_len(view_rng);
    f32 scroll_ratio = (f32)rng_s64_len(view_rng) / (f32)scroll_indices;
    
    ui_set_next_pref_size(axis, ui_pct(1.f, 0.f));
    ui_set_next_pref_size(flip_axis, flip_axis_size);
    ui_set_next_child_layout_axis(axis);
    ui_set_next_hover_cursor(OS_CURSOR_HAND);
    UI_Box *container = ui_make_box_from_stringf(UI_BOX_DRAW_BACKGROUND, "###container_%s", name.data);
    UI_Parent(container)
        UI_PrefSize(flip_axis, ui_pct(1.f, 0.f)) {
        ui_set_next_pref_size(axis, ui_text_dim(0.f, 1.f));
        UI_Signal top_sig = ui_button(str8_lit("<###top_arrow"));
        if (ui_clicked(top_sig) || ui_dragging(top_sig)) {
            new_pt.idx -= 1;
        }

        ui_set_next_pref_size(axis, ui_pct(1.f, 0.f));
        ui_set_next_background_color(V4(.24f, .25f, .25f, 1.f));
        ui_set_next_hover_cursor(OS_CURSOR_HAND);
        UI_Box *thumb_container = ui_make_box_from_stringf(UI_BOX_CLICKABLE | UI_BOX_DRAW_BACKGROUND, "###thumb_container", name.data);
        v2 thumb_container_dim = rect_dim(thumb_container->rect);
            
        UI_Signal scroll_sig = ui_signal_from_box(thumb_container);

        if (ui_clicked(scroll_sig)) {
            v2 scroll_pos = ui_mouse() - thumb_container->rect.p0;
            new_pt.idx = (s64)(scroll_pos[axis] / (thumb_container_dim[axis] / (f32)view_indices));
        }

        f32 thumb_pos = thumb_container_dim[axis] * ((f32)scroll_pt.idx / (f32)scroll_indices);
        ui_set_next_parent(thumb_container);
        ui_set_next_fixed_xy(axis, thumb_pos);
        ui_set_next_fixed_xy(flip_axis, 0.f);
        ui_set_next_pref_size(axis, ui_pct(scroll_ratio, 0.f));
        ui_set_next_background_color(V4(.4f, .4f, .4f, 1.f));
        ui_set_next_hover_cursor(OS_CURSOR_HAND);
        UI_Box *thumb_box = ui_make_box_from_stringf(UI_BOX_CLICKABLE | UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_HOT_EFFECTS | UI_BOX_DRAW_ACTIVE_EFFECTS, "###thumb", name.data);
        UI_Signal thumb_sig = ui_signal_from_box(thumb_box);
        if (ui_dragging(thumb_sig)) {
            v2 scroll_pos = ui_mouse() - thumb_container->rect.p0;
            new_pt.idx = (s64)(scroll_pos[axis] / (thumb_container_dim[axis] / (f32)view_indices));
        }

        ui_set_next_pref_size(axis, ui_text_dim(0.f, 1.f));
        UI_Signal bottom_sig = ui_button(str8_lit(">###bottom_arrow"));
        if (ui_clicked(bottom_sig) || ui_dragging(bottom_sig)) {
            new_pt.idx += 1;
        }
    }

    new_pt.idx = Clamp(new_pt.idx, 0, view_indices - 1);

    return new_pt;
}


#if 0
internal void ui_slider(String8 name, f32 *value, f32 min, f32 max) {
    v2 mouse = ui_mouse();
    UI_Box *found = ui_find_box(hash);
    bool hot = false;
    if (found) {
        hot = point_in_rect(mouse, found->rect);
    }
    UI_Box *box = ui_make_box_from_string(name, 0);
    if (found) ui_copy_box(box, found);

    if (ui_active_id() == hash) {
        if (!input_mouse(MOUSE_LEFT)) {
            ui_set_active(0);
        }
    } else {
        if (hot && input_mouse_down(MOUSE_LEFT)) {
            ui_set_active(hash);
        }
    }

    if (ui_active_id() == hash) {
        f32 ratio = (CLAMP(mouse.x, box->rect.x0, box->rect.x1) - box->rect.x0) / rect_width(box->rect);
        f32 c = min + (max - min) * ratio;
        *value = c;
    }

    int name_count = (int)strlen(name);
    f32 width = 2.0f * get_string_width(name, name_count, ui_face);
    f32 height = ui_face->glyph_height;

    ui_advance();

    box->rect = make_rect(ui_state.cursor, V2(width, height));

    *value = CLAMP(*value, min, max);
    
    f32 ratio = (*value - min) / (max - min);
    f32 overlay_width = ratio * rect_width(box->rect);

    Rect overlay_rect = { box->rect.x0, box->rect.y0 };
    overlay_rect.x1 = overlay_rect.x0 + overlay_width;
    overlay_rect.y1 = box->rect.y1;

    v4 bg_color = V4(1.0f, 1.0f, 1.0f, 1.0f);
    v4 overlay_color = V4(0.1f, 0.1f, 0.1f, 0.2f);
    v4 fg_color = V4(0.0f, 0.0f, 0.0f, 1.0f);
    v4 border_color = V4(0.0f, 0.0f, 0.0f, 1.0f);

    draw_rect(box->rect, bg_color);
    draw_rect_outline(box->rect, border_color);
    draw_text(name, name_count, ui_state.cursor, fg_color, ui_face);
    draw_rect(overlay_rect, overlay_color);

    ui_state.cursor.y = box->rect.y1;
}
#endif

internal UI_Signal ui_directory_list(String8 directory) {
    ui_set_next_child_layout_axis(AXIS_Y);
    UI_Signal signal = ui_button(directory);
    UI_Parent(signal.box)
    UI_PrefWidth(ui_pct(1.0f, 1.0f)) UI_PrefHeight(ui_text_dim(0.f, 1.f)) {
        OS_File file;
        OS_Handle find_handle = os_find_first_file(ui_build_arena(), directory, &file);
        if (find_handle) {
            do {
                UI_Signal file_signal = ui_label(file.file_name);
                if (ui_clicked(file_signal)) {
                    printf("%s\n", file.file_name.data);
                }
            } while (os_find_next_file(ui_build_arena(), find_handle, &file));
            os_find_close(find_handle);
        }
    }
    return signal;
}

internal void ui_row_begin(String8 name) {
    ui_set_next_pref_width(ui_pct(1.0f, 1.0f));
    ui_set_next_pref_height(ui_children_sum(1.0f));
    ui_set_next_child_layout_axis(AXIS_X);
    UI_Box *row = ui_make_box_from_string(UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_BORDER, name);
    ui_push_parent(row);
}

internal void ui_row_end() {
    ui_pop_parent();
}
