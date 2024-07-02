#include "draw.h"

internal void draw_vertex(v2 position, v2 uv, v4 color) {
    R_2D_Vertex v = {position, uv, color};
    r_d3d11_state->ui_vertices.push(v);
}

internal void draw_glyph(Glyph g, Face *face, v4 color, v2 position) {
    f32 x0 = position.x + g.bl;
    f32 x1 = x0 + g.bx;
    f32 y0 = position.y - g.bt + face->ascend;
    f32 y1 = y0 + g.by;

    f32 tw = g.bx / (f32)face->width;
    f32 th = g.by / (f32)face->height;
    f32 tx = g.to;
    f32 ty = 0.0f;

    draw_vertex(V2(x0, y1), V2(tx,      ty + th), color);
    draw_vertex(V2(x0, y0), V2(tx,      ty),      color);
    draw_vertex(V2(x1, y0), V2(tx + tw, ty),      color);
    draw_vertex(V2(x1, y1), V2(tx + tw, ty + th), color);
}

internal void draw_string(String8 string, Face *font_face, v4 color, v2 offset) {
    v2 cursor = offset;
    for (u64 i = 0; i < string.count; i++) {
        u8 c = string.data[i];
        if (c == '\n') {
            cursor.x = offset.x;
            cursor.y += font_face->glyph_height;
            continue;
        }
        Glyph g = font_face->glyphs[c];
        draw_glyph(g, font_face, color, cursor);
        cursor.x += g.ax;
    }
}

internal void draw_rect(Rect rect, v4 color) {
    v2 uv = V2(0.f, 0.f);
    draw_vertex(V2(rect.x0, rect.y0), uv, color);
    draw_vertex(V2(rect.x0, rect.y1), uv, color);
    draw_vertex(V2(rect.x1, rect.y1), uv, color);
    draw_vertex(V2(rect.x1, rect.y0), uv, color);
}

internal void draw_rect_outline(Rect rect, v4 color) {
    draw_rect(make_rect(rect.x0, rect.y0, rect_width(rect), 1), color);
    draw_rect(make_rect(rect.x0, rect.y0, 1, rect_height(rect)), color);
    draw_rect(make_rect(rect.x1 - 1, rect.y0, 1, rect_height(rect)), color);
    draw_rect(make_rect(rect.x0, rect.y1 - 1, rect_width(rect), 1), color);
}
