// @todo add options for adjusting editor focus (top, center, bottom)
// void ensure_cursor_in_editor(Editor *editor, Cursor cursor) {
//     f32 cursor_top = cursor.line * editor->face->glyph_height - editor->editor_offset.y;
//     f32 cursor_bottom = cursor_top + editor->face->glyph_height;
//     if (cursor_top < 0) {
//         editor->editor_offset.y += (int)cursor_top;
//     } else if (cursor_bottom >= rect_height(editor->rect)) {
//         f32 d = cursor_bottom - rect_height(editor->rect);
//         editor->editor_offset.yset.y += d;
//     }
// }

internal void editor_set_cursor(Hoc_Editor *editor, Cursor cursor) {
    // ensure_cursor_in_editor(editor, cursor);
    editor->cursor = cursor;
}

internal void editor_move_lines(Hoc_Editor *editor, s64 lines) {
    s64 line = editor->cursor.line + lines;
    line = Clamp(line, 0, buffer_get_line_count(editor->buffer) - 1);
    Cursor cursor = get_cursor_from_line(editor->buffer, line);
    editor_set_cursor(editor, cursor);
}

internal void write_buffer(Hoc_Buffer *buffer) {
    // String8 buffer_string = buffer_to_string_apply_line_endings(buffer);
    Arena *arena = make_arena(get_malloc_allocator());
    String8 buffer_string = buffer_to_string(arena, buffer);
    OS_Handle file_handle = os_open_file(buffer->file_name, OS_ACCESS_WRITE);
    if (os_valid_handle(file_handle)) {
        os_write_file(file_handle, buffer_string.data, buffer_string.count);
        os_close_handle(file_handle);
        buffer->edited = false;
    } else {
        printf("Could not open file '%s'\n", buffer->file_name.data);
    }
    arena_release(arena);
}

HOC_COMMAND(nil_command) {
}

HOC_COMMAND(self_insert) {
    Assert(view->type == GUI_VIEW_EDITOR);
    GUI_Editor *gui_editor = &view->editor;
    Hoc_Editor *editor = view->editor.editor;
    String8 text_input = gui_editor->active_text_input;
    if (text_input.data) {
        if (editor->mark_active) {
            s64 start = editor->cursor.position < editor->mark.position ? editor->cursor.position : editor->mark.position;
            s64 end = editor->cursor.position < editor->mark.position ? editor->mark.position : editor->cursor.position;
            buffer_replace_region(editor->buffer, text_input, start, end);
            editor->mark_active = false;
            editor_set_cursor(editor, get_cursor_from_position(editor->buffer, start));
        } else {
            buffer_insert_string(editor->buffer, editor->cursor.position, text_input);
            // buffer_record_insert(editor->buffer, editor->cursor.position, text_input);
            editor_set_cursor(editor, get_cursor_from_position(editor->buffer, editor->cursor.position + text_input.count));
        }
        // if (editor->buffer->post_self_insert_hook) editor->buffer->post_self_insert_hook(text_input);
    }
    editor->modified = true;
}

HOC_COMMAND(quit_hoc) {
    quit_hoc_application();
}

HOC_COMMAND(newline) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    Cursor cursor = editor->cursor;
    buffer_insert_single(editor->buffer, cursor.position, '\n');
    cursor.position++;
    cursor.col = 0;
    cursor.line++;
    editor_set_cursor(editor, cursor);
}

HOC_COMMAND(backward_char) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->cursor.position > 0) {
        Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position - 1);
        editor_set_cursor(editor, cursor);
    }
}

HOC_COMMAND(forward_char) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->cursor.position < buffer_get_length(editor->buffer)) {
        Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position + 1);
        editor_set_cursor(editor, cursor);
    }
}

HOC_COMMAND(forward_word) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 buffer_length = buffer_get_length(editor->buffer);
    u8 first = buffer_at(editor->buffer, editor->cursor.position);
    s64 position = editor->cursor.position;
    // eat whitespace
    if (isspace(first)) {
        for (; position < buffer_length; position++) {
            char c = buffer_at(editor->buffer, position);
            if (!isspace(c)) {
                break;
            }
        }
    }
    for (; position < buffer_length; position++) {
        char c = buffer_at(editor->buffer, position);
        if (isspace(c)) {
            break;
        }
    }

    editor_set_cursor(editor, get_cursor_from_position(editor->buffer, position));
}

HOC_COMMAND(backward_word) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 buffer_length = buffer_get_length(editor->buffer);
    char first = buffer_at(editor->buffer, editor->cursor.position);
    s64 position = editor->cursor.position;
    // eat whitespace
    if (isspace(first)) {
        for (; position >= 0; position--) {
            char c = buffer_at(editor->buffer, position);
            if (!isspace(c)) {
                break;
            }
        }
    }
    for (position = position - 1; position >= 0; position--) {
        char c = buffer_at(editor->buffer, position);
        if (isspace(c)) {
            break;
        }
    }
    editor_set_cursor(editor, get_cursor_from_position(editor->buffer, position));
}

HOC_COMMAND(backward_paragraph) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    for (s64 line = editor->cursor.line - 1; line > 0; line--) {
        s64 start = editor->buffer->line_starts[line - 1];
        s64 end = editor->buffer->line_starts[line];
        bool blank_line = true;
        for (s64 position = start; position < end; position++) {
            char c = buffer_at(editor->buffer, position);
            if (!isspace(c)) {
                blank_line = false;
                break;
            }
        }
        if (blank_line) {
            editor_set_cursor(editor, get_cursor_from_position(editor->buffer, start));
            break;
        }
    }
}

HOC_COMMAND(forward_paragraph) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    for (s64 line = editor->cursor.line + 1; line < buffer_get_line_count(editor->buffer) - 1; line++) {
        s64 start = editor->buffer->line_starts[line];
        s64 end = editor->buffer->line_starts[line + 1];
        bool blank_line = true;
        for (s64 position = start; position < end; position++) {
            char c = buffer_at(editor->buffer, position);
            if (!isspace(c)) {
                blank_line = false;
                break;
            }
        }
        if (blank_line) {
            editor_set_cursor(editor, get_cursor_from_position(editor->buffer, start));
            break;
        }
    }    
}

HOC_COMMAND(previous_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->cursor.line > 0) {
        s64 position = get_position_from_line(editor->buffer, editor->cursor.line - 1);
        Cursor cursor = get_cursor_from_position(editor->buffer, position);
        editor_set_cursor(editor, cursor);
    }
}

HOC_COMMAND(next_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->cursor.line < buffer_get_line_count(editor->buffer) - 1) {
        s64 position = get_position_from_line(editor->buffer, editor->cursor.line + 1);
        Cursor cursor = get_cursor_from_position(editor->buffer, position);
        editor_set_cursor(editor, cursor);
    }
}

HOC_COMMAND(page_up) {
    GUI_Editor *gui_editor = &view->editor;
    Hoc_Editor *editor = gui_editor->editor;
    s64 prev_line = editor->cursor.line;
    editor_move_lines(editor, -20);
    s64 lines_moved = editor->cursor.line - prev_line;
    gui_editor->scroll_pt += lines_moved * editor->face->glyph_height;
    // gui_editor->box->view_offset_target.y = -editor->cursor.line * gui_editor->editor->face->glyph_height;
}

HOC_COMMAND(page_down) {
    GUI_Editor *gui_editor = &view->editor;
    Hoc_Editor *editor = gui_editor->editor;
    s64 prev_line = editor->cursor.line;
    editor_move_lines(editor, 20);
    s64 lines_moved = editor->cursor.line - prev_line;
    gui_editor->scroll_pt += lines_moved * editor->face->glyph_height;
    // gui_editor->box->view_offset_target.y = -editor->cursor.line * gui_editor->editor->face->glyph_height;
}

HOC_COMMAND(backward_delete_char) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->mark_active) {
        if (editor->cursor.position != editor->mark.position) {
            Cursor start = editor->mark.position < editor->cursor.position ? editor->mark : editor->cursor;
            Cursor end = editor->mark.position < editor->cursor.position ? editor->cursor : editor->mark;
            buffer_delete_region(editor->buffer, start.position, end.position);
            editor_set_cursor(editor, get_cursor_from_position(editor->buffer, start.position));
        }
        editor->mark_active = false;
    } else if (editor->cursor.position > 0) {
        // buffer_record_delete(editor->buffer, editor->cursor.position - 1, editor->cursor.position);
        buffer_delete_single(editor->buffer, editor->cursor.position);
        Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position - 1);
        editor_set_cursor(editor, cursor);
    }
}

HOC_COMMAND(forward_delete_char) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->mark_active) {
        if (editor->cursor.position != editor->mark.position) {
            Cursor start = editor->mark.position < editor->cursor.position ? editor->mark : editor->cursor;
            Cursor end = editor->mark.position < editor->cursor.position ? editor->cursor : editor->mark;
            buffer_delete_region(editor->buffer, start.position, end.position);
            editor_set_cursor(editor, get_cursor_from_position(editor->buffer, start.position));
        }
        editor->mark_active = false;
    } else if (editor->cursor.position < buffer_get_length(editor->buffer)) {
        // buffer_record_delete(editor->buffer, editor->cursor.position, editor->cursor.position + 1);
        buffer_delete_single(editor->buffer, editor->cursor.position + 1);
        Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position);
        editor_set_cursor(editor, cursor);
    }
}

HOC_COMMAND(backward_delete_word) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 buffer_length = buffer_get_length(editor->buffer);
    char first = buffer_at(editor->buffer, editor->cursor.position);
    s64 position = editor->cursor.position;
    // eat whitespace
    if (isspace(first)) {
        for (; position >= 0; position--) {
            char c = buffer_at(editor->buffer, position);
            if (!isspace(c)) {
                break;
            }
        }
    }
    for (position = position - 1; position >= 0; position--) {
        char c = buffer_at(editor->buffer, position);
        if (isspace(c)) {
            break;
        }
    }

    position = Clamp(position, 0, buffer_length);

    // buffer_record_delete(editor->buffer, position, editor->cursor.position);
    buffer_delete_region(editor->buffer, position, editor->cursor.position);
    editor_set_cursor(editor, get_cursor_from_position(editor->buffer, position));
}

HOC_COMMAND(forward_delete_word) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 buffer_length = buffer_get_length(editor->buffer);
    char first = buffer_at(editor->buffer, editor->cursor.position);
    s64 position = editor->cursor.position;
    // eat whitespace
    if (isspace(first)) {
        for (; position < buffer_length; position++) {
            char c = buffer_at(editor->buffer, position);
            if (!isspace(c)) {
                break;
            }
        }
    }
    for (; position < buffer_length; position++) {
        char c = buffer_at(editor->buffer, position);
        if (isspace(c)) {
            break;
        }
    }

    position = Clamp(position, 0, buffer_length);

    // buffer_record_delete(editor->buffer, editor->cursor.position, position);
    buffer_delete_region(editor->buffer, editor->cursor.position, position);
    editor_set_cursor(editor, get_cursor_from_position(editor->buffer, editor->cursor.position));
}

HOC_COMMAND(kill_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 start = editor->cursor.position;
    s64 end = start + buffer_get_line_length(editor->buffer, editor->cursor.line);
    if (end - start == 0) end += 1;
    buffer_delete_region(editor->buffer, start, end);
    editor_set_cursor(editor, editor->cursor);
}

HOC_COMMAND(open_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 line_begin = get_position_from_line(editor->buffer, editor->cursor.line + 1);
    buffer_insert_single(editor->buffer, line_begin, '\n');
    editor_set_cursor(editor, get_cursor_from_line(editor->buffer, editor->cursor.line + 1));
}

HOC_COMMAND(save_buffer) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    write_buffer(editor->buffer);
    editor->modified = false;
}

HOC_COMMAND(set_mark) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    editor->mark_active = true;
    editor->mark = editor->cursor;    
}

HOC_COMMAND(goto_beginning_of_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 position = get_position_from_line(editor->buffer, editor->cursor.line);
    editor_set_cursor(editor, get_cursor_from_position(editor->buffer, position));
}

HOC_COMMAND(goto_end_of_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 position = get_position_from_line(editor->buffer, editor->cursor.line) + buffer_get_line_length(editor->buffer, editor->cursor.line);
    editor_set_cursor(editor, get_cursor_from_position(editor->buffer, position));
}

HOC_COMMAND(goto_first_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    editor_set_cursor(editor, get_cursor_from_position(editor->buffer, 0));
}

HOC_COMMAND(goto_last_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    editor_set_cursor(editor, get_cursor_from_position(editor->buffer, buffer_get_length(editor->buffer)));
}

HOC_COMMAND(copy) {
    Hoc_Editor *editor = view->editor.editor;
    Rng_S64 rng = rng_s64(editor->cursor.position, editor->mark.position);
    if (rng_s64_len(rng)  > 0) {
        Arena *scratch = arena_alloc(get_malloc_allocator(), rng_s64_len(rng) + 1);
        String8 string = buffer_to_string_range(scratch, editor->buffer, rng);
        os_set_clipboard_text(string);
        arena_release(scratch);
    }
}

HOC_COMMAND(paste) {
    Hoc_Editor *editor = view->editor.editor;
    Arena *scratch = make_arena(get_malloc_allocator());
    String8 string = os_get_clipboard_text(scratch);
    buffer_insert_string(editor->buffer, editor->cursor.position, string);
    editor_set_cursor(editor, get_cursor_from_position(editor->buffer, editor->cursor.position + string.count));
    arena_release(scratch);
}

internal GUI_View *make_gui_file_system();
internal void gui_file_system_load_path(GUI_File_System *fs, String8 full_path);

HOC_COMMAND(find_file) {
    GUI_Editor *gui_editor = &view->editor;
    Hoc_Editor *editor = gui_editor->editor;
    GUI_View *fs_view = make_gui_file_system();
    GUI_File_System *fs = &fs_view->fs;
    fs->current_editor = gui_editor;

    //@Note Initializes current path and sub-files
    String8 current_path = editor->buffer->file_path;
    gui_file_system_load_path(fs, current_path);
}
