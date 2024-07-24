#include "ui_core.h"
#include "ui_widgets.h"

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
    UI_Box *box = ui_make_box_from_string(UI_BOX_CLICKABLE | UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_BORDER | UI_BOX_DRAW_TEXT | UI_BOX_DRAW_HOT_EFFECTS | UI_BOX_DRAW_ACTIVE_EFFECTS, string);
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
    draw_string(edit_string, box->font_face, box->text_color, text_position);

    String8 string_before_cursor = edit_string;
    string_before_cursor.count = (u64)draw_data->cursor;
    v2 c_pos = text_position + measure_string_size(string_before_cursor, box->font_face);
    Rect c_rect = make_rect(c_pos.x, c_pos.y, 2.f, box->font_face->glyph_height);
    if (ui_key_match(box->key, ui_state->focus_active_box_key)) {
        draw_rect(c_rect, box->text_color);
    }
}

internal UI_Signal ui_line_edit(String8 name, void *buffer, u64 max_buffer_capacity, u64 *buffer_pos,  u64 *buffer_count) {
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

struct UI_Scroll_Bar_Draw {
    Axis2 axis;
    f32 thumb_position;
    f32 view_bounds;
};

internal UI_BOX_CUSTOM_DRAW_PROC(ui_draw_scroll_bar) {
    UI_Scroll_Bar_Draw *draw_data = (UI_Scroll_Bar_Draw *)user_data;
    draw_rect(box->rect, box->background_color);
    Axis2 axis = draw_data->axis;

    v2 scroll_bar_dim = rect_dim(box->rect);
    f32 view_ratio =  scroll_bar_dim[axis] / draw_data->view_bounds;

    v2 thumb_position = box->rect.p0;
    thumb_position[draw_data->axis] += draw_data->thumb_position * view_ratio;
    v2 thumb_size = scroll_bar_dim;
    thumb_size[axis] = scroll_bar_dim[axis] * view_ratio; 

    Rect thumb_rect = make_rect(thumb_position.x, thumb_position.y, thumb_size.x, thumb_size.y);

    draw_rect(thumb_rect, V4(.65f, .65f, .65f, 1.f));
}

internal UI_Signal ui_scroll_bar(String8 name, Axis2 axis, f32 scroll_pt, f32 view_bounds) {
    ui_set_next_fixed_width(20.f);
    ui_set_next_pref_height(ui_pct(1.f, 0.f));
    UI_Box *scroll_bar = ui_make_box_from_stringf(UI_BOX_CLICKABLE | UI_BOX_DRAW_BACKGROUND, "scroll_bar_%s", name.data);
    UI_Signal signal = ui_signal_from_box(scroll_bar);

    UI_Scroll_Bar_Draw *draw_data = push_array(ui_build_arena(), UI_Scroll_Bar_Draw, 1);
    draw_data->axis = axis;
    draw_data->thumb_position = scroll_pt;
    draw_data->view_bounds = view_bounds;
    ui_set_custom_draw(scroll_bar, ui_draw_scroll_bar, draw_data);
    return signal;
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
    ui_set_next_child_layout(AXIS_Y);
    UI_Signal signal = ui_button(directory);
    UI_Parent(signal.box)
    UI_PrefWidth(ui_pct(1.0f, 1.0f)) UI_PrefHeight(ui_text_dim(0.0f, 1.0f)) {
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
    ui_set_next_child_layout(AXIS_X);
    UI_Box *row = ui_make_box_from_string(UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_BORDER, name);
    ui_push_parent(row);
}

internal void ui_row_end() {
    ui_pop_parent();
}
