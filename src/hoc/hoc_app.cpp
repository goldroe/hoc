global Hoc_Application *hoc_app;
global bool window_should_close;

internal Hoc_Application *make_hoc_application() {
    Arena *arena = make_arena(get_malloc_allocator());
    Hoc_Application *app = push_array(arena, Hoc_Application, 1);
    app->arena = arena;
    app->buffer_id_counter = 0;
    app->view_id_counter = 0;
    return app;
}

internal void quit_hoc_application() {
    window_should_close = true;
}

