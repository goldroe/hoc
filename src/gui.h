#ifndef GUI_H
#define GUI_H

enum GUI_View_Type {
    GUI_VIEW_NIL,
    GUI_VIEW_EDITOR,
    GUI_VIEW_FILE_SYSTEM,
};

struct GUI_Editor {
    Hoc_Editor *editor;
    UI_Box *box;
    String8 active_text_input;
    f32 scroll_pt;
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


#endif // GUI_H
