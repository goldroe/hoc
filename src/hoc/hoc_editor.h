#ifndef HOC_EDITOR_H
#define HOC_EDITOR_H

struct Hoc_Editor {
    Arena *arena;
    Hoc_Editor *prev;
    Hoc_Editor *next;
    Hoc_Buffer *buffer;

    Face *font;
    Cursor cursor;
    b32 mark_active;
    Cursor mark;
    s64 pref_col;
    b32 modified;
};

#endif // HOC_EDITOR_H
