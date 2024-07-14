#ifndef HOC_EDITOR_H
#define HOC_EDITOR_H

struct Hoc_Editor {
    Arena *arena;
    Hoc_Editor *prev;
    Hoc_Editor *next;
    Hoc_Buffer *buffer;

    Face *face;
    Cursor cursor;
    b32 mark_active;
    Cursor mark;

    f32 cursor_dt;
};

#endif // HOC_EDITOR_H
