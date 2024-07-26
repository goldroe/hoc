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
};

struct GUI_File_System {
    Arena *arena;
    GUI_Editor *current_editor;
    u8  path_buffer[2048];
    u64 path_len;
    u64 path_pos;
    
    // String8 normalized_path;
    Arena *cached_files_arena;
    OS_File *cached_files;
    int cached_file_count;
};

struct GUI_View {
    Arena *arena;
    GUI_View_ID id;
    GUI_View *prev;
    GUI_View *next;
    GUI_View_Type type;
    Key_Map *key_map;
    b32 to_be_destroyed;

    UI_Scroll_Pos scroll_pos;
    
    union {
        GUI_Editor editor;
        GUI_File_System fs;
    };
};


#endif // GUI_H
