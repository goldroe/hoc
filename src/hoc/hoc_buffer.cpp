#include "hoc_buffer.h"

#define DEFAULT_GAP_SIZE 1024

#define PTR_IN_GAP(Ptr, Buffer) (Ptr >= Buffer->text + Buffer->gap_start && Ptr < Buffer->text + Buffer->gap_end)
#define GAP_SIZE(Buffer) (Buffer->gap_end - Buffer->gap_start)
#define BUFFER_SIZE(Buffer) (Buffer->end - GAP_SIZE(Buffer))

internal void modify_buffer(Hoc_Buffer *buffer) {
    buffer->modified = true;
}

internal void write_buffer(Hoc_Buffer *buffer) {
    Arena *scratch = make_arena(get_virtual_allocator());
    String8 full_path = path_join(scratch, buffer->file_path, buffer->file_name);
    String8 buffer_string = buffer_to_string(scratch, buffer, true);
    OS_Handle file_handle = os_open_file(full_path, OS_ACCESS_WRITE);
    if (os_valid_handle(file_handle)) {
        os_write_file(file_handle, buffer_string.data, buffer_string.count);
        os_close_handle(file_handle);
        buffer->modified = false;
    }
    arena_release(scratch);
}

internal String8 buffer_to_string(Arena *arena, Hoc_Buffer *buffer, bool apply_line_ends) {
    s64 line_count = buffer_get_line_count(buffer);
    s64 buffer_length = buffer_get_length(buffer);
    if (apply_line_ends) {
        buffer_length += (line_count + 1)*(buffer->line_end == LINE_ENDING_CRLF); 
    }

    String8 result{};
    result.data = push_array_no_zero(arena, u8, buffer_length + 1);
    result.count = buffer_length;
    s64 str_idx = 0;
    for (s64 line = 0; line < line_count; line += 1) {
        s64 start = get_position_from_line(buffer, line);
        s64 line_length = buffer_get_line_length(buffer, line);
        s64 end = start + line_length;
        for (s64 i = start; i < end; i += 1) {
            result.data[str_idx++] = buffer_at(buffer, i);
        }
        if (apply_line_ends) {
            if (buffer->line_end != LINE_ENDING_CR) {
                if (buffer->line_end == LINE_ENDING_CRLF) result.data[str_idx++] = '\r';
                result.data[str_idx++] = '\n';
            } else {
                result.data[str_idx++] = '\r';
            }
        } else {
            result.data[str_idx++] = '\n';
        }
    }
    result.data[str_idx] = 0;
    return result;
}

internal String8 buffer_to_string_range(Arena *arena, Hoc_Buffer *buffer, Rng_S64 rng) {
    String8 result = str8_zero();
    s64 count = rng_s64_len(rng);
    result.data = push_array(arena, u8, count + 1);
    result.count = count;
    for (s64 i = 0, rng_idx = rng.min; rng_idx < rng.max; i += 1, rng_idx += 1) {
        result.data[i] = buffer_at(buffer, rng_idx);
    }
    result.data[count] = 0;
    return result;
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

internal Line_End detect_line_ending(String8 string) {
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
    buffer->file_name = path_strip_file_name(arena_alloc(base_allocator, file_name.count + 1), file_name);
    buffer->text = string.data;
    buffer->gap_start = 0;
    buffer->gap_end = 0;
    buffer->end = string.count;
    buffer->line_end = LINE_ENDING_LF;
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
        Line_End line_end = detect_line_ending(buffer_string);
        if (line_end != LINE_ENDING_LF) {
            String8 adjusted{};
            remove_crlf(buffer_string.data, buffer_string.count, &adjusted.data, &adjusted.count);
            free(file_data);
            buffer_string = adjusted;
        }
        buffer_init_contents(buffer, file_name, buffer_string);
        buffer->line_end = line_end;
    } else {
        String8 string = str8_zero();
        string.data = (u8 *)malloc(buffer->end);
        string.count = DEFAULT_GAP_SIZE;
        buffer_init_contents(buffer, file_name, string);
        buffer->line_end = LINE_ENDING_LF;
    }
    
    DLLPushBack(hoc_app->buffers.first, hoc_app->buffers.last, buffer, next, prev);
    hoc_app->buffers.count += 1;
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
    modify_buffer(buffer);
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
    modify_buffer(buffer);
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
    modify_buffer(buffer);
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
    modify_buffer(buffer);
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

internal s64 buffer_indentation_from_line(Hoc_Buffer *buffer, s64 line) {
    s64 result = 0;
    s64 start = get_position_from_line(buffer, line);
    for (s64 i = start, end = start + buffer_get_line_length(buffer, line); i < end; i += 1) {
        if (buffer_at(buffer, i) != ' ') {
            break;
        }
        result += 1;
    }
    return result;
}

internal Auto_Array<Rng_S64> buffer_find_text_matches(Hoc_Buffer *buffer, String8 string) {
    Auto_Array<Rng_S64> match_ranges;

    for (s64 i = 0, length = buffer_get_length(buffer); i < length; i++) {
        u64 rem = length - i;
        if (rem < string.count) {
            break;
        }
        
        if (buffer_at(buffer, i) == string.data[0]) {
            bool matches = true;
            for (u64 j = 0; j < string.count; j++) {
                if (buffer_at(buffer, i + j) != string.data[j]) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                Rng_S64 rng = rng_s64(i, i + string.count);
                match_ranges.push(rng);
            }
        }
    }

    return match_ranges;
}
