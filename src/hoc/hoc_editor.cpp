internal Hoc_Editor *make_editor() {
    Arena *arena = make_arena(get_malloc_allocator());
    Hoc_Editor *result = push_array(arena, Hoc_Editor, 1);
    result->arena = arena;
    return result;
}

