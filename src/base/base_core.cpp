
internal Rng_U64 rng_u64(u64 min, u64 max) { Rng_U64 result; result.min = min; result.max = max; return result; }
internal u64 rng_u64_count(Rng_U64 rng) { u64 result = rng.max - rng.min; return result; }

internal v2 rect_dim(Rect rect) {
    v2 result;
    result.x = rect.x1 - rect.x0;
    result.y = rect.y1 - rect.y0;
    return result;
}

internal inline Rect make_rect(f32 x, f32 y, f32 w, f32 h) {
    Rect result;
    result.x0 = x;
    result.y0 = y;
    result.x1 = x + w;
    result.y1 = y + h;
    return result;
}

internal inline Rect make_rect_center(v2 position, v2 size) {
    Rect result = make_rect(position.x - size.x/2.0f, position.y - size.y/2.0f, size.x, size.y);
    return result;
}

internal inline f32 rect_height(Rect rect) {f32 result = rect.y1 - rect.y0; return result;}
internal inline f32 rect_width(Rect rect) {f32 result = rect.x1 - rect.x0; return result;}

internal inline bool operator==(Rect a, Rect b) {
    return a.x0 == b.x0 && a.x1 == b.x1 && a.y0 == b.y0 && a.y1 == b.y1;
}

internal inline bool operator!=(Rect a, Rect b) {
    return !(a == b);
}

