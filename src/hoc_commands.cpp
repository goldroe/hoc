
// @todo add options for adjusting view focus (top, center, bottom)
// void ensure_cursor_in_view(View *view, Cursor cursor) {
//     f32 cursor_top = cursor.line * view->face->glyph_height - view->view_offset.y;
//     f32 cursor_bottom = cursor_top + view->face->glyph_height;
//     if (cursor_top < 0) {
//         view->view_offset.y += (int)cursor_top;
//     } else if (cursor_bottom >= rect_height(view->rect)) {
//         f32 d = cursor_bottom - rect_height(view->rect);
//         view->view_offset.yset.y += d;
//     }
// }

void view_set_cursor(View *view, Cursor cursor) {
    // ensure_cursor_in_view(view, cursor);
    view->cursor = cursor;
}

void write_buffer(Buffer *buffer) {
    // String8 buffer_string = buffer_to_string_apply_line_endings(buffer);
    Arena *arena = make_arena(get_malloc_allocator());
    String8 buffer_string = buffer_to_string(arena, buffer);
    OS_Handle file_handle = os_open_file(buffer->file_name, OS_ACCESS_WRITE);
    if (os_valid_handle(file_handle)) {
        printf("%s\n", buffer_string.data);
        os_write_file(file_handle, buffer_string.data, buffer_string.count);
        os_close_handle(file_handle);
    } else {
        printf("Could not open file '%s'\n", buffer->file_name.data);
    }
    arena_release(arena);
}


COMMAND(nil_command) {
}

COMMAND(self_insert) {
    View *view = get_active_view();
    if (view->active_text_input.data) {
        if (view->mark_active) {
            s64 start = view->cursor.position < view->mark.position ? view->cursor.position : view->mark.position;
            s64 end = view->cursor.position < view->mark.position ? view->mark.position : view->cursor.position;
            buffer_replace_region(view->buffer, view->active_text_input, start, end);
            view->mark_active = false;
            view_set_cursor(view, get_cursor_from_position(view->buffer, start));
        } else {
            buffer_insert_string(view->buffer, view->cursor.position, view->active_text_input);
            // buffer_record_insert(view->buffer, view->cursor.position, view->active_text_input);
            view_set_cursor(view, get_cursor_from_position(view->buffer, view->cursor.position + view->active_text_input.count));
        }
        // if (view->buffer->post_self_insert_hook) view->buffer->post_self_insert_hook(active_text_input);
    }
}


COMMAND(quit_hoc) {
    quit_hoc_application();
}

// COMMAND(quit_selection) {
//     get_active_view()->mark_active = false;
// }

COMMAND(newline) {
    View *view = get_active_view();
    Cursor cursor = view->cursor;
    buffer_insert_single(view->buffer, cursor.position, '\n');
    cursor.position++;
    cursor.col = 0;
    cursor.line++;
    view_set_cursor(view, cursor);
}


COMMAND(backward_char) {
    View *view = get_active_view();
    if (view->cursor.position > 0) {
        Cursor cursor = get_cursor_from_position(view->buffer, view->cursor.position - 1);
        view_set_cursor(view, cursor);
    }
}

COMMAND(forward_char) {
    View *view = get_active_view();
    if (view->cursor.position < buffer_get_length(view->buffer)) {
        Cursor cursor = get_cursor_from_position(view->buffer, view->cursor.position + 1);
        view_set_cursor(view, cursor);
    }
}

COMMAND(forward_word) {
    View *view = get_active_view();
    s64 buffer_length = buffer_get_length(view->buffer);
    u8 first = buffer_at(view->buffer, view->cursor.position);
    s64 position = view->cursor.position;
    // eat whitespace
    if (isspace(first)) {
        for (; position < buffer_length; position++) {
            char c = buffer_at(view->buffer, position);
            if (!isspace(c)) {
                break;
            }
        }
    }
    for (; position < buffer_length; position++) {
        char c = buffer_at(view->buffer, position);
        if (isspace(c)) {
            break;
        }
    }

    view_set_cursor(view, get_cursor_from_position(view->buffer, position));
}

COMMAND(backward_word) {
    View *view = get_active_view();
    s64 buffer_length = buffer_get_length(view->buffer);
    char first = buffer_at(view->buffer, view->cursor.position);
    s64 position = view->cursor.position;
    // eat whitespace
    if (isspace(first)) {
        for (; position >= 0; position--) {
            char c = buffer_at(view->buffer, position);
            if (!isspace(c)) {
                break;
            }
        }
    }
    for (position = position - 1; position >= 0; position--) {
        char c = buffer_at(view->buffer, position);
        if (isspace(c)) {
            break;
        }
    }
    view_set_cursor(view, get_cursor_from_position(view->buffer, position));
}

COMMAND(backward_paragraph) {
    View *view = get_active_view();
    for (s64 line = view->cursor.line - 1; line > 0; line--) {
        s64 start = view->buffer->line_starts[line - 1];
        s64 end = view->buffer->line_starts[line];
        bool blank_line = true;
        for (s64 position = start; position < end; position++) {
            char c = buffer_at(view->buffer, position);
            if (!isspace(c)) {
                blank_line = false;
                break;
            }
        }
        if (blank_line) {
            view_set_cursor(view, get_cursor_from_position(view->buffer, start));
            break;
        }
    }
}

COMMAND(forward_paragraph) {
    View *view = get_active_view();
    for (s64 line = view->cursor.line + 1; line < buffer_get_line_count(view->buffer) - 1; line++) {
        s64 start = view->buffer->line_starts[line];
        s64 end = view->buffer->line_starts[line + 1];
        bool blank_line = true;
        for (s64 position = start; position < end; position++) {
            char c = buffer_at(view->buffer, position);
            if (!isspace(c)) {
                blank_line = false;
                break;
            }
        }
        if (blank_line) {
            view_set_cursor(view, get_cursor_from_position(view->buffer, start));
            break;
        }
    }    
}

COMMAND(previous_line) {
    View *view = get_active_view();
    if (view->cursor.line > 0) {
        s64 position = get_position_from_line(view->buffer, view->cursor.line - 1);
        Cursor cursor = get_cursor_from_position(view->buffer, position);
        view_set_cursor(view, cursor);
    }
}

COMMAND(next_line) {
    View *view = get_active_view();
    if (view->cursor.line < buffer_get_line_count(view->buffer) - 1) {
        s64 position = get_position_from_line(view->buffer, view->cursor.line + 1);
        Cursor cursor = get_cursor_from_position(view->buffer, position);
        view_set_cursor(view, cursor);
    }
}

COMMAND(backward_delete_char) {
    View *view = get_active_view();
    if (view->mark_active) {
        if (view->cursor.position != view->mark.position) {
            Cursor start = view->mark.position < view->cursor.position ? view->mark : view->cursor;
            Cursor end = view->mark.position < view->cursor.position ? view->cursor : view->mark;
            buffer_delete_region(view->buffer, start.position, end.position);
            view_set_cursor(view, get_cursor_from_position(view->buffer, start.position));
        }
        view->mark_active = false;
    } else if (view->cursor.position > 0) {
        // buffer_record_delete(view->buffer, view->cursor.position - 1, view->cursor.position);
        buffer_delete_single(view->buffer, view->cursor.position);
        Cursor cursor = get_cursor_from_position(view->buffer, view->cursor.position - 1);
        view_set_cursor(view, cursor);
    }
}

COMMAND(forward_delete_char) {
    View *view = get_active_view();
    if (view->mark_active) {
        if (view->cursor.position != view->mark.position) {
            Cursor start = view->mark.position < view->cursor.position ? view->mark : view->cursor;
            Cursor end = view->mark.position < view->cursor.position ? view->cursor : view->mark;
            buffer_delete_region(view->buffer, start.position, end.position);
            view_set_cursor(view, get_cursor_from_position(view->buffer, start.position));
        }
        view->mark_active = false;
    } else if (view->cursor.position < buffer_get_length(view->buffer)) {
        // buffer_record_delete(view->buffer, view->cursor.position, view->cursor.position + 1);
        buffer_delete_single(view->buffer, view->cursor.position + 1);
        Cursor cursor = get_cursor_from_position(view->buffer, view->cursor.position);
        view_set_cursor(view, cursor);
    }
}

COMMAND(backward_delete_word) {
    View *view = get_active_view();
    s64 buffer_length = buffer_get_length(view->buffer);
    char first = buffer_at(view->buffer, view->cursor.position);
    s64 position = view->cursor.position;
    // eat whitespace
    if (isspace(first)) {
        for (; position >= 0; position--) {
            char c = buffer_at(view->buffer, position);
            if (!isspace(c)) {
                break;
            }
        }
    }
    for (position = position - 1; position >= 0; position--) {
        char c = buffer_at(view->buffer, position);
        if (isspace(c)) {
            break;
        }
    }

    position = Clamp(position, 0, buffer_length);

    // buffer_record_delete(view->buffer, position, view->cursor.position);
    buffer_delete_region(view->buffer, position, view->cursor.position);
    view_set_cursor(view, get_cursor_from_position(view->buffer, position));
}

COMMAND(forward_delete_word) {
    View *view = get_active_view();
    s64 buffer_length = buffer_get_length(view->buffer);
    char first = buffer_at(view->buffer, view->cursor.position);
    s64 position = view->cursor.position;
    // eat whitespace
    if (isspace(first)) {
        for (; position < buffer_length; position++) {
            char c = buffer_at(view->buffer, position);
            if (!isspace(c)) {
                break;
            }
        }
    }
    for (; position < buffer_length; position++) {
        char c = buffer_at(view->buffer, position);
        if (isspace(c)) {
            break;
        }
    }

    position = Clamp(position, 0, buffer_length);

    // buffer_record_delete(view->buffer, view->cursor.position, position);
    buffer_delete_region(view->buffer, view->cursor.position, position);
    view_set_cursor(view, get_cursor_from_position(view->buffer, view->cursor.position));
}

COMMAND(kill_line) {
    View *view = get_active_view();
    s64 start = view->cursor.position;
    s64 end = start + buffer_get_line_length(view->buffer, view->cursor.line);
    if (end - start == 0) end += 1;
    buffer_delete_region(view->buffer, start, end);
    view_set_cursor(view, view->cursor);
}

COMMAND(open_line) {
    View *view = get_active_view();
    s64 line_begin = get_position_from_line(view->buffer, view->cursor.line + 1);
    buffer_insert_single(view->buffer, line_begin, '\n');
    view_set_cursor(view, get_cursor_from_line(view->buffer, view->cursor.line + 1));
}

COMMAND(save_buffer) {
    write_buffer(get_active_view()->buffer);
}

COMMAND(set_mark) {
    View *view = get_active_view();
    view->mark_active = true;
    view->mark = view->cursor;    
}

COMMAND(goto_beginning_of_line) {
    View *view = get_active_view();
    s64 position = get_position_from_line(view->buffer, view->cursor.line);
    view_set_cursor(view, get_cursor_from_position(view->buffer, position));
}

COMMAND(goto_end_of_line) {
    View *view = get_active_view();
    s64 position = get_position_from_line(view->buffer, view->cursor.line) + buffer_get_line_length(view->buffer, view->cursor.line);
    view_set_cursor(view, get_cursor_from_position(view->buffer, position));
}

COMMAND(goto_first_line) {
    View *view = get_active_view();
    view_set_cursor(view, get_cursor_from_position(view->buffer, 0));
}

COMMAND(goto_last_line) {
    View *view = get_active_view();
    view_set_cursor(view, get_cursor_from_position(view->buffer, buffer_get_length(view->buffer)));
}

internal void set_active_gui(GUI_View gui);
internal void gui_file_system_start(String8 initial_path);

COMMAND(find_file) {
    set_active_gui(GUI_FILE_SYSTEM);
    View *view = get_active_view();
    gui_file_system_start(view->buffer->file_path);
}

COMMAND(find_file_exit) {
    set_active_gui(GUI_NIL);
}
