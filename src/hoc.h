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

struct Editor_List {
    Hoc_Editor *first;
    Hoc_Editor *last;
    Hoc_Editor *free_editors;
    int count;
};

enum GUI_View_Type {
    GUI_VIEW_NIL,
    GUI_VIEW_EDITOR,
    GUI_VIEW_FILE_SYSTEM,
};

struct GUI_Editor {
    Hoc_Editor *editor;
    UI_Box *box;
    v2 panel_dim;
    String8 active_text_input;
};

struct GUI_File_System {
    Arena *arena;
    GUI_Editor *current_editor;
    u8  path_buffer[2048];
    u64 path_len;
    u64 path_pos;
    String8 sub_file_paths[2048];
    int sub_file_count;
};

struct GUI_View {
    GUI_View_ID id;
    GUI_View *prev;
    GUI_View *next;
    GUI_View_Type type;
    Key_Map *key_map;
    union {
        GUI_Editor editor;
        GUI_File_System fs;
    };
};

#endif // HOC_H
