#ifndef HOC_BUFFER_H
#define HOC_BUFFER_H

typedef u32 View_ID;
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

struct Buffer {
    Buffer_ID id;
    Buffer *prev;
    Buffer *next;
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


struct View {
    Arena *arena;
    View_ID id;
    View *prev;
    View *next;
    Buffer *buffer;

    v2 panel_dim;
    UI_Box *box;

    Face *face;
    Cursor cursor;
    b32 mark_active;
    Cursor mark;

    String8 active_text_input;
    Key_Map *key_map;
};

internal Buffer *make_buffer(String8 file_name);
internal Buffer *make_buffer_from_file(String8 file_name);

internal s64 buffer_get_line_length(Buffer *buffer, s64 line);
internal s64 buffer_get_line_count(Buffer *buffer);
internal s64 buffer_get_length(Buffer *buffer);

internal u8 buffer_at(Buffer *buffer, s64 position);
internal void buffer_insert_single(Buffer *buffer, s64 position, u8 c);
internal void buffer_insert_text(Buffer *buffer, s64 position, String8 text);
internal void buffer_delete_single(Buffer *buffer, s64 position);
internal void buffer_replace_region(Buffer *buffer, String8 string, s64 start, s64 end);
internal void buffer_delete_region(Buffer *buffer, s64 start, s64 end);
internal void buffer_clear(Buffer *buffer);

internal Cursor get_cursor_from_position(Buffer *buffer, s64 position);
internal s64 get_position_from_line(Buffer *buffer, s64 line);
internal Cursor get_cursor_from_line(Buffer *buffer, s64 line);

internal String8 buffer_to_string(Arena *arena, Buffer *buffer);
internal String8 buffer_to_string_apply_line_endings(Buffer *buffer);
internal String8 buffer_to_string_span(Buffer *buffer, Span span);

internal void buffer_record_insert(Buffer *buffer, s64 position, String8 text);
internal void buffer_record_delete(Buffer *buffer, s64 start, s64 end);

internal void buffer_update_line_starts(Buffer *buffer);


#endif // HOC_BUFFER_H
