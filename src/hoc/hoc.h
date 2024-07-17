#ifndef HOC_H
#define HOC_H

struct GUI_View;
typedef u32 GUI_View_ID;

#define HOC_COMMAND(name) void name(GUI_View *view)
typedef HOC_COMMAND(Hoc_Command_Proc);

struct Hoc_Command {
    String8 name;
    Hoc_Command_Proc *procedure;
};

#define KEY_MOD_ALT 1<<10
#define KEY_MOD_CONTROL 1<<9
#define KEY_MOD_SHIFT 1<<8
#define MAX_KEY_MAPPINGS (1 << (8 + 3))
struct Key_Map {
    String8 name;
    Hoc_Command *mappings;
};

struct Hoc_Editor;
struct Hoc_Buffer;

struct Buffer_List {
    Hoc_Buffer *first;
    Hoc_Buffer *last;
    int count;
};

#endif // HOC_H

