internal Hoc_Editor *make_editor() {
    Arena *arena = make_arena(get_malloc_allocator());
    Hoc_Editor *result = push_array(arena, Hoc_Editor, 1);
    result->arena = arena;
    return result;
}

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
