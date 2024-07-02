#pragma once

struct String8 {
    u8 *data;
    u64 count;
};

#define str8_lit(S) {(u8 *)(S), sizeof((S)) - 1}

internal String8 str8(u8 *c, u64 count);
internal String8 str8_zero();
internal u64 cstr8_length(const char *c);
internal String8 str8_cstring(const char *c);
