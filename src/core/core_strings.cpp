#include "core_strings.h"

internal u64 cstr8_length(const char *c) {
    if (c == nullptr) return 0;
    u64 result = 0;
    while (*c++) {
        result++;
    }
    return result;
}

internal String8 str8_zero() {
    String8 result = {0, 0};
    return result;
}

internal String8 str8(u8 *c, u64 count) {
    String8 result = {(u8 *)c, count};
    return result;
}

internal String8 str8_cstring(const char *c) {
    String8 result;
    result.count = cstr8_length(c);
    result.data = (u8 *)c;
    return result;
}

internal String8 str8_copy(Arena *arena, String8 string) {
    String8 result;
    result.count = string.count;
    result.data = (u8 *)arena_push(arena, result.count + 1);
    MemoryCopy(result.data, string.data, string.count);
    result.data[result.count] = 0;
    return result;
}

internal String8 str8_concat(Arena *arena, String8 first, String8 second) {
    String8 result;
    result.count = first.count + second.count;
    result.data = push_array(arena, u8, result.count + 1);
    MemoryCopy(result.data, first.data, first.count);
    MemoryCopy(result.data + first.count, second.data, second.count);
    result.data[result.count] = 0;
    return result;
}

internal void str8_append(String8 *first, String8 second) {
    String8 temp;
    temp.count = first->count + second.count;
    temp.data = (u8 *)malloc(temp.count + 1);
    MemoryCopy(temp.data, first->data, first->count);
    MemoryCopy(temp.data + first->count, second.data, second.count);
    temp.data[temp.count] = 0;
    *first = temp;
}

internal bool str8_eq(String8 first, String8 second) {
    if (first.count != second.count) return false;
    for (u64 i = 0; i < first.count; i++) {
        if (first.data[i] != second.data[i]) {
            return false;
        }
    }
    return true;
}