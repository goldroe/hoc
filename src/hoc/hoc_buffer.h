#ifndef HOC_BUFFER_H
#define HOC_BUFFER_H

typedef u32 Editor_ID;
typedef u32 Buffer_ID;

enum Line_Ending {
    LINE_ENDING_NONE,
    LINE_ENDING_LF,
    LINE_ENDING_CR,
    LINE_ENDING_CRLF,
};

struct Cursor {
    s64 position;
    s64 line;
    s64 col;
};

struct Span {
    s64 start;
    s64 end;
};

struct Hoc_Buffer {
    Buffer_ID id;
    Hoc_Buffer *prev;
    Hoc_Buffer *next;
    String8 file_path;
    String8 file_name;
    u8 *text;
    s64 gap_start;
    s64 gap_end;
    s64 end;

    Line_Ending line_ending;
    Auto_Array<s64> line_starts;
    // Self_Insert_Hook post_self_insert_hook;
};

internal Hoc_Buffer *make_buffer(String8 file_name);
internal Hoc_Buffer *make_buffer_from_file(String8 file_name);

internal s64 buffer_get_line_length(Hoc_Buffer *buffer, s64 line);
internal s64 buffer_get_line_count(Hoc_Buffer *buffer);
internal s64 buffer_get_length(Hoc_Buffer *buffer);

internal u8 buffer_at(Hoc_Buffer *buffer, s64 position);
internal void buffer_insert_single(Hoc_Buffer *buffer, s64 position, u8 c);
internal void buffer_insert_text(Hoc_Buffer *buffer, s64 position, String8 text);
internal void buffer_delete_single(Hoc_Buffer *buffer, s64 position);
internal void buffer_replace_region(Hoc_Buffer *buffer, String8 string, s64 start, s64 end);
internal void buffer_delete_region(Hoc_Buffer *buffer, s64 start, s64 end);
internal void buffer_clear(Hoc_Buffer *buffer);

internal Cursor get_cursor_from_position(Hoc_Buffer *buffer, s64 position);
internal s64 get_position_from_line(Hoc_Buffer *buffer, s64 line);
internal Cursor get_cursor_from_line(Hoc_Buffer *buffer, s64 line);

internal String8 buffer_to_string(Arena *arena, Hoc_Buffer *buffer);
internal String8 buffer_to_string_apply_line_endings(Hoc_Buffer *buffer);
internal String8 buffer_to_string_span(Hoc_Buffer *buffer, Span span);

internal void buffer_record_insert(Hoc_Buffer *buffer, s64 position, String8 text);
internal void buffer_record_delete(Hoc_Buffer *buffer, s64 start, s64 end);

internal void buffer_update_line_starts(Hoc_Buffer *buffer);

#endif // HOC_BUFFER_H
