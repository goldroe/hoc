#include "hoc_buffer.h"

#define DEFAULT_GAP_SIZE 1024

#define PTR_IN_GAP(Ptr, Buffer) (Ptr >= Buffer->text + Buffer->gap_start && Ptr < Buffer->text + Buffer->gap_end)
#define GAP_SIZE(Buffer) (Buffer->gap_end - Buffer->gap_start)
#define BUFFER_SIZE(Buffer) (Buffer->end - GAP_SIZE(Buffer))

internal void push_buffer(Hoc_Buffer *buffer) {
    if (hoc_app->buffers.first) {
        buffer->prev = hoc_app->buffers.last;
        hoc_app->buffers.last->next = buffer;
        hoc_app->buffers.last = buffer;
    } else {
        hoc_app->buffers.first = hoc_app->buffers.last = buffer;
    }
}

internal String8 buffer_to_string(Arena *arena, Hoc_Buffer *buffer) {
    s64 buffer_length = buffer_get_length(buffer);
    String8 result{};
    result.data = push_array(arena, u8, buffer_length + 1);
    MemoryCopy(result.data, buffer->text, buffer->gap_start);
    MemoryCopy(result.data + buffer->gap_start, buffer->text + buffer->gap_end, buffer->end - buffer->gap_end);
    result.count = buffer_length;
    result.data[buffer_length] = 0;
    return result;
}

internal String8 buffer_to_string_span(Hoc_Buffer *buffer, Span span) {
    String8 result{};
    s64 span_length = span.end - span.start;
    Assert(span.start >= 0 && span.end >= 0);
    Assert(span_length >= 0);
    if (span.end < buffer_get_length(buffer)) {
        result.data = (u8 *)malloc(span_length + 1);
        result.data[span_length] = 0;
        for (s64 i = 0; i < span_length; i++) {
            result.data[i] = buffer_at(buffer, span.start + i);
        }
    }
    result.count = span_length;
    return result;
}

internal String8 buffer_to_string_apply_line_endings(Hoc_Buffer *buffer) {
    s64 buffer_length = buffer_get_length(buffer); // @todo wrong?
    s64 new_buffer_length = buffer_length + ((buffer->line_ending == LINE_ENDING_CRLF) ? buffer_get_line_count(buffer) : 0);
    String8 buffer_string;
    buffer_string.data = (u8 *)malloc(new_buffer_length + 1);
    buffer_string.count = new_buffer_length;
    u8 *dest = buffer_string.data;
    for (s64 position = 0; position < buffer_length; position++) {
        u8 c = buffer_at(buffer, position);
        Assert(c >= 0 && c <= 127);
        if (c == '\n') {
            switch (buffer->line_ending) {
            case LINE_ENDING_LF:
                *dest++ = '\n';
                break;
            case LINE_ENDING_CR:
                *dest++ = '\r';
                break;
            case LINE_ENDING_CRLF:
                *dest++ = '\r';
                *dest++ = '\n';
                break;
            }
        } else {
            *dest++ = c;
        }
    }
    *dest = 0;
    buffer_string.count = dest - buffer_string.data;
    return buffer_string;
}

internal s64 buffer_get_line_length(Hoc_Buffer *buffer, s64 line) {
    s64 length = buffer->line_starts[line + 1] - buffer->line_starts[line] - 1;
    return length;
}

internal s64 buffer_get_line_count(Hoc_Buffer *buffer) {
    s64 result = buffer->line_starts.count - 1;
    return result;
}

internal s64 buffer_get_length(Hoc_Buffer *buffer) {
    s64 result = BUFFER_SIZE(buffer);
    return result;
}

internal void remove_crlf(u8 *data, s64 count, u8 **out_data, u64 *out_count) {
    u8 *result = (u8 *)malloc(count);
    u8 *src = data;
    u8 *src_end = data + count;
    u8 *dest = result;
    while (src < data + count) {
        switch (*src) {
        default:
            *dest++ = *src++;
            break;
        case '\r':
            src++;
            if (src < src_end && *src == '\n') src++;
            *dest++ = '\n';
            break;
        case '\n':
            src++;
            if (src < src_end && *src == '\r') src++;
            *dest++ = '\n';
            break;
        }
    }

    u64 new_count = dest - result;
    result = (u8 *)realloc(result, new_count);
    if (out_data) *out_data = result;
    if (out_count) *out_count = new_count;
}

internal Line_Ending detect_line_ending(String8 string) {
    for (u64 i = 0; i < string.count; i++) {
        switch (string.data[i]) {
        case '\r':
            if (i + 1 < string.count && string.data[i + 1] == '\n') {
                return LINE_ENDING_CRLF;
            } else {
                return LINE_ENDING_CR;
            }
        case '\n':
            return LINE_ENDING_LF;
        }
    }
    return LINE_ENDING_LF;
}

internal void buffer_init_contents(Hoc_Buffer *buffer, String8 file_name, String8 string) {
    Base_Allocator *base_allocator = get_malloc_allocator();
    buffer->file_path = path_strip_dir_name(arena_alloc(base_allocator, file_name.count + 1), file_name);
    printf("dir_name: %s\n", buffer->file_path.data);
    buffer->file_name = str8_copy(arena_alloc(base_allocator, file_name.count + 1), file_name);
    buffer->text = string.data;
    buffer->gap_start = 0;
    buffer->gap_end = 0;
    buffer->end = string.count;
    buffer->line_ending = LINE_ENDING_LF;
    buffer_update_line_starts(buffer);
}

internal Hoc_Buffer *make_buffer(String8 file_name) {
    Hoc_Buffer *buffer = new Hoc_Buffer();
    OS_Handle handle = os_open_file(file_name, OS_ACCESS_READ);
    if (os_valid_handle(handle)) {
        u8 *file_data = nullptr;
        u64 file_size = os_read_entire_file(handle, (void **)&file_data);
        os_close_handle(handle);
        Assert(file_data);

        String8 buffer_string = {file_data, file_size};
        Line_Ending line_ending = detect_line_ending(buffer_string);
        if (line_ending != LINE_ENDING_LF) {
            String8 adjusted{};
            remove_crlf(buffer_string.data, buffer_string.count, &adjusted.data, &adjusted.count);
            free(file_data);
            buffer_string = adjusted;
        }
        buffer_init_contents(buffer, file_name, buffer_string);
        buffer->line_ending = line_ending;
    } else {
        String8 string = str8_zero();
        string.data = (u8 *)malloc(buffer->end);
        string.count = DEFAULT_GAP_SIZE;
        buffer_init_contents(buffer, file_name, string);
        buffer->line_ending = LINE_ENDING_LF;
    }
    push_buffer(buffer);

    return buffer;
}

internal s64 buffer_position_logical(Hoc_Buffer *buffer, s64 position) {
    if (position > buffer->gap_start) {
        position -= GAP_SIZE(buffer);
    }
    return position;
}

internal u8 buffer_at(Hoc_Buffer *buffer, s64 position) {
    s64 index = position;
    if (index >= buffer->gap_start) {
        index += GAP_SIZE(buffer);
    }
    u8 c = 0;
    if (buffer_get_length(buffer) > 0) {
       c = buffer->text[index]; 
    }
    return c;
}

internal void buffer_update_line_starts(Hoc_Buffer *buffer) {
    buffer->line_starts.reset_count();
    buffer->line_starts.push(0);
    
    u8 *text = buffer->text;
    for (;;) {
        if (text >= buffer->text + buffer->end) break;
        if (PTR_IN_GAP(text, buffer)) {
            text = buffer->text + buffer->gap_end;
            continue;
        }

        bool newline = false;
        switch (*text) {
        default:
            text++;
            break;
        case '\r':
            text++;
            if (!PTR_IN_GAP(text, buffer) && (text < buffer->text + buffer->end)) {
                if (*text == '\n') text++;
            }
            newline = true;
            break;
        case '\n':
            text++;
            if (!PTR_IN_GAP(text, buffer) && (text < buffer->text + buffer->end)) {
                if (*text == '\r') text++;
            }
            newline = true;
            break;
        }
        if (newline) {
            s64 position = text - buffer->text;
            position = buffer_position_logical(buffer, position);
            buffer->line_starts.push(position);
        }
    }
    buffer->line_starts.push(BUFFER_SIZE(buffer) + 1);
}

internal void buffer_grow(Hoc_Buffer *buffer, s64 gap_size) {
    s64 size1 = buffer->gap_start;
    s64 size2 = buffer->end - buffer->gap_end;
    u8 *data = (u8 *)calloc(buffer->end + gap_size, 1);
    memcpy(data, buffer->text, buffer->gap_start);
    memset(data + size1, '_', gap_size);
    memcpy(data + buffer->gap_start + gap_size, buffer->text + buffer->gap_end, buffer->end - buffer->gap_end);
    free(buffer->text);
    buffer->text = data;
    buffer->gap_end += gap_size;
    buffer->end += gap_size;
}

internal void buffer_shift_gap(Hoc_Buffer *buffer, s64 new_gap) {
    u8 *temp = (u8 *)malloc(buffer->end);
    s64 gap_start = buffer->gap_start;
    s64 gap_end = buffer->gap_end;
    s64 gap_size = gap_end - gap_start;
    s64 size = buffer->end;
    if (new_gap > gap_start) {
        memcpy(temp, buffer->text, new_gap);
        memcpy(temp + gap_start, buffer->text + gap_end, new_gap - gap_start);
        memcpy(temp + new_gap + gap_size, buffer->text + new_gap + gap_size, size - (new_gap + gap_size));
    } else {
        memcpy(temp, buffer->text, new_gap);
        memcpy(temp + new_gap + gap_size, buffer->text + new_gap, gap_start - new_gap);
        memcpy(temp + new_gap + gap_size + (gap_start - new_gap), buffer->text + gap_end, size - gap_end);
    }
    free(buffer->text);
    buffer->text = temp;
    buffer->gap_start = new_gap;
    buffer->gap_end = new_gap + gap_size;
}

internal void buffer_ensure_gap(Hoc_Buffer *buffer) {
    if (buffer->gap_end - buffer->gap_start == 0) {
        buffer_grow(buffer, DEFAULT_GAP_SIZE);
    }
}

internal void buffer_delete_region(Hoc_Buffer *buffer, s64 start, s64 end) {
    Assert(start < end);
    if (buffer->gap_start != start) {
        buffer_shift_gap(buffer, start);
    }
    buffer->gap_end += (end - start);
    buffer_update_line_starts(buffer);
}

internal void buffer_delete_single(Hoc_Buffer *buffer, s64 position) {
    buffer_delete_region(buffer, position - 1, position);
}

internal void buffer_insert_single(Hoc_Buffer *buffer, s64 position, u8 c) {
    buffer_ensure_gap(buffer);
    if (buffer->gap_start != position) {
        buffer_shift_gap(buffer, position);
    }
    buffer->text[position] = c;
    buffer->gap_start++;
    buffer_update_line_starts(buffer);
}

internal void buffer_insert_string(Hoc_Buffer *buffer, s64 position, String8 string) {
    if (GAP_SIZE(buffer) < (s64)string.count) {
        buffer_grow(buffer, string.count);
    }
    if (buffer->gap_start != position) {
        buffer_shift_gap(buffer, position);
    }
    MemoryCopy(buffer->text + position, string.data, string.count);
    buffer->gap_start += string.count;
    buffer_update_line_starts(buffer);
}

internal void buffer_replace_region(Hoc_Buffer *buffer, String8 string, s64 start, s64 end) {
    s64 region_size = end - start;
    buffer_delete_region(buffer, start, end);
    if (GAP_SIZE(buffer) < (s64)string.count) {
        buffer_grow(buffer, string.count);
    }
    memcpy(buffer->text + buffer->gap_start, string.data, string.count);
    buffer->gap_start += string.count;
    buffer_update_line_starts(buffer);
}

internal void buffer_clear(Hoc_Buffer *buffer) {
    buffer->gap_start = 0;
    buffer->gap_end = buffer->end;
}

internal Cursor get_cursor_from_position(Hoc_Buffer *buffer, s64 position) {
    Cursor cursor = {};
    for (int line = 0; line < buffer->line_starts.count - 1; line++) {
        s64 start = buffer->line_starts[line];
        s64 end = buffer->line_starts[line + 1];
        if (start <= position && position < end) {
            cursor.line = line;
            cursor.col = position - start;
            break;
        }
    }
    cursor.position = position;
    return cursor;
}

internal s64 get_position_from_line(Hoc_Buffer *buffer, s64 line) {
    s64 position = buffer->line_starts[line];
    return position;
}

internal Cursor get_cursor_from_line(Hoc_Buffer *buffer, s64 line) {
    s64 position = get_position_from_line(buffer, line);
    Cursor cursor = get_cursor_from_position(buffer, position);
    return cursor;
}
