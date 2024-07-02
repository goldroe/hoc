#ifndef HOC_H
#define HOC_H

#define COMMAND(name) void name()
typedef void (*Hoc_Command_Proc)(void);

struct Hoc_Command {
    String8 name;
    Hoc_Command_Proc procedure;
};

#define KEY_MOD_ALT 1<<10
#define KEY_MOD_CONTROL 1<<9
#define KEY_MOD_SHIFT 1<<8
#define MAX_KEY_MAPPINGS (1 << (8 + 3))
struct Key_Map {
    String8 name;
    Hoc_Command *mappings;
};

struct View;
struct Buffer;

struct Buffer_List {
    Buffer *first;
    Buffer *last;
    int count;
};

struct View_List {
    View *first;
    View *last;
    View *free_views;
    int count;
};

struct Hoc_Application {
    Arena *arena;

    //@Note View and Buffer lists
    View *active_view;
    View_List views;
    int view_id_counter;

    Buffer_List buffers;
    int buffer_id_counter;
};

internal View *get_active_view();
internal void quit_hoc_application();

#endif // HOC_H
