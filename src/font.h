#ifndef FONT_H
#define FONT_H

struct Glyph {
    f32 ax;
    f32 ay;
    f32 bx;
    f32 by;
    f32 bt;
    f32 bl;
    f32 to;
};

struct Face {
    String8 font_name;
    Glyph glyphs[256];
    int width;
    int height;
    int max_bmp_height;
    f32 ascend;
    f32 descend;
    int bbox_height;
    f32 glyph_width;
    f32 glyph_height;
    void *texture;
};

#endif // FONT_H
