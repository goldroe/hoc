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
            Cursor cursor = get_cursor_from_position(editor->buffer, start);
            editor_set_cursor(editor, cursor);
            ensure_cursor_in_view(view, cursor);
        } else {
            buffer_insert_string(editor->buffer, editor->cursor.position, text_input);
            // buffer_record_insert(editor->buffer, editor->cursor.position, text_input);
            Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position + text_input.count);
            editor_set_cursor(editor, cursor);
            ensure_cursor_in_view(view, cursor);
        }
        // if (editor->buffer->post_self_insert_hook) editor->buffer->post_self_insert_hook(text_input);
    }
}

HOC_COMMAND(quit_hoc) {
    quit_hoc_application();
}

HOC_COMMAND(newline) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    Cursor prev_cursor = editor->cursor;
    s64 indent = buffer_indentation_from_line(editor->buffer, prev_cursor.line);
    buffer_insert_single(editor->buffer, prev_cursor.position, '\n');

    //@Note add previous line indentation
    s64 pos = prev_cursor.position + 1;
    for (s64 i = 0; i < indent; i++) {
        buffer_insert_single(editor->buffer, pos, ' ');
        pos += 1;
    }
    
    Cursor cursor = get_cursor_from_position(editor->buffer, pos);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
}

HOC_COMMAND(backward_char) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->cursor.position > 0) {
        Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position - 1);
        editor_set_cursor(editor, cursor);
        ensure_cursor_in_view(view, cursor);
    }
}

HOC_COMMAND(forward_char) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->cursor.position < buffer_get_length(editor->buffer)) {
        Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position + 1);
        editor_set_cursor(editor, cursor);
        ensure_cursor_in_view(view, cursor);
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

    Cursor cursor = get_cursor_from_position(editor->buffer, position);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
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
    Cursor cursor = get_cursor_from_position(editor->buffer, position);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
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
            Cursor cursor = get_cursor_from_position(editor->buffer, start);
            editor_set_cursor(editor, cursor);
            ensure_cursor_in_view(view, cursor);
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
            Cursor cursor = get_cursor_from_position(editor->buffer, start);
            editor_set_cursor(editor, cursor);
            ensure_cursor_in_view(view, cursor);
            break;
        }
    }    
}

HOC_COMMAND(previous_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->cursor.line > 0) {
        s64 line = editor->cursor.line - 1; 
        s64 col = Clamp(editor->pref_col, 0, buffer_get_line_length(editor->buffer, line));
        s64 position = get_position_from_line(editor->buffer, line) + col;
        Cursor cursor = get_cursor_from_position(editor->buffer, position);
        s64 pref_col = editor->pref_col;
        editor_set_cursor(editor, cursor);
        ensure_cursor_in_view(view, cursor);
        editor->pref_col = pref_col;
    }
}

HOC_COMMAND(next_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->cursor.line < buffer_get_line_count(editor->buffer) - 1) {
        s64 line = editor->cursor.line + 1; 
        s64 col = Clamp(editor->pref_col, 0, buffer_get_line_length(editor->buffer, line));
        s64 position = get_position_from_line(editor->buffer, line) + col;
        Cursor cursor = get_cursor_from_position(editor->buffer, position);
        s64 pref_col = editor->pref_col;
        editor_set_cursor(editor, cursor);
        ensure_cursor_in_view(view, cursor);
        editor->pref_col = pref_col;
    }
}

HOC_COMMAND(page_up) {
    GUI_Editor *gui_editor = &view->editor;
    Hoc_Editor *editor = gui_editor->editor;
    v2 code_area_dim = rect_dim(gui_editor->box->rect);
    s64 viewable_lines = (s64)ceil(code_area_dim.y / gui_editor->box->font->glyph_height);
    editor_move_lines(editor, -viewable_lines);
    view->scroll_pos.y.idx = editor->cursor.line;
}

HOC_COMMAND(page_down) {
    GUI_Editor *gui_editor = &view->editor;
    Hoc_Editor *editor = gui_editor->editor;
    v2 code_area_dim = rect_dim(gui_editor->box->rect);
    s64 viewable_lines = (s64)ceil(code_area_dim.y / gui_editor->box->font->glyph_height);
    editor_move_lines(editor, viewable_lines);
    view->scroll_pos.y.idx = editor->cursor.line;
}

HOC_COMMAND(backward_delete_char) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    if (editor->mark_active) {
        if (editor->cursor.position != editor->mark.position) {
            Cursor start = editor->mark.position < editor->cursor.position ? editor->mark : editor->cursor;
            Cursor end = editor->mark.position < editor->cursor.position ? editor->cursor : editor->mark;
            buffer_delete_region(editor->buffer, start.position, end.position);
            Cursor cursor = get_cursor_from_position(editor->buffer, start.position);
            editor_set_cursor(editor, cursor);
            ensure_cursor_in_view(view, cursor);
        }
        editor->mark_active = false;
    } else if (editor->cursor.position > 0) {
        // buffer_record_delete(editor->buffer, editor->cursor.position - 1, editor->cursor.position);
        buffer_delete_single(editor->buffer, editor->cursor.position);
        Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position - 1);
        editor_set_cursor(editor, cursor);
        ensure_cursor_in_view(view, cursor);
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
            Cursor cursor = get_cursor_from_position(editor->buffer, start.position);
            editor_set_cursor(editor, cursor);
            ensure_cursor_in_view(view, cursor);
        }
        editor->mark_active = false;
    } else if (editor->cursor.position < buffer_get_length(editor->buffer)) {
        // buffer_record_delete(editor->buffer, editor->cursor.position, editor->cursor.position + 1);
        buffer_delete_single(editor->buffer, editor->cursor.position + 1);
        Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position);
        editor_set_cursor(editor, cursor);
        ensure_cursor_in_view(view, cursor);
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
    Cursor cursor = get_cursor_from_position(editor->buffer, position);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
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
    Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
}

HOC_COMMAND(kill_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 start = editor->cursor.position;
    s64 end = start + buffer_get_line_length(editor->buffer, editor->cursor.line);
    if (end - start == 0) end += 1;
    buffer_delete_region(editor->buffer, start, end);
    editor_set_cursor(editor, editor->cursor);
    ensure_cursor_in_view(view, editor->cursor);
}

HOC_COMMAND(open_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 line_begin = get_position_from_line(editor->buffer, editor->cursor.line + 1);
    buffer_insert_single(editor->buffer, line_begin, '\n');
    Cursor cursor = get_cursor_from_line(editor->buffer, editor->cursor.line + 1);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
}

HOC_COMMAND(save_buffer) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    write_buffer(editor->buffer);
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
    Cursor cursor = get_cursor_from_position(editor->buffer, position);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
}

HOC_COMMAND(goto_end_of_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    s64 position = get_position_from_line(editor->buffer, editor->cursor.line) + buffer_get_line_length(editor->buffer, editor->cursor.line);
    Cursor cursor = get_cursor_from_position(editor->buffer, position);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
}

HOC_COMMAND(goto_first_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    Cursor cursor = get_cursor_from_position(editor->buffer, 0);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
}

HOC_COMMAND(goto_last_line) {
    Assert(view->type == GUI_VIEW_EDITOR);
    Hoc_Editor *editor = view->editor.editor;
    Cursor cursor = get_cursor_from_position(editor->buffer, buffer_get_length(editor->buffer));
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
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
    Cursor cursor = get_cursor_from_position(editor->buffer, editor->cursor.position + string.count);
    editor_set_cursor(editor, cursor);
    ensure_cursor_in_view(view, cursor);
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

HOC_COMMAND(search_text) {
    GUI_Editor *gui_editor = &view->editor;
    gui_editor->searching = true;
    gui_editor->search_cursor = gui_editor->editor->cursor;
}

HOC_COMMAND(nil_command) {
}
