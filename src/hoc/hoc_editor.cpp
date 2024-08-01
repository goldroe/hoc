internal Hoc_Editor *make_editor() {
    Arena *arena = make_arena(get_malloc_allocator());
    Hoc_Editor *result = push_array(arena, Hoc_Editor, 1);
    result->arena = arena;
    return result;
}

internal void editor_set_cursor(Hoc_Editor *editor, Cursor cursor) {
    editor->cursor = cursor;
    editor->pref_col = cursor.col;
}

internal void editor_move_lines(Hoc_Editor *editor, s64 lines) {
    s64 line = editor->cursor.line + lines;
    line = Clamp(line, 0, buffer_get_line_count(editor->buffer) - 1);
    Cursor cursor = get_cursor_from_line(editor->buffer, line);
    editor_set_cursor(editor, cursor);
}
