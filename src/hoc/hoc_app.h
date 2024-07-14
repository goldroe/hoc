#ifndef HOC_APP_H
#define HOC_APP_H

struct GUI_View_List {
    GUI_View *first;
    GUI_View *last;
    int count;
};

struct Hoc_Application {
    Arena *arena;

    String8 current_directory;
    
    Buffer_List buffers;
    int buffer_id_counter;

    GUI_View_List gui_views;
    int view_id_counter;
};

internal void quit_hoc_application();

#endif // HOC_APP_H
