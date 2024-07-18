internal ALLOC_COMMIT_SIG(virtual_allocator_commit) {
    void *result = nullptr;
    result = VirtualAlloc(NULL, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (result == NULL) {
        int error = GetLastError();
        fprintf(stderr, "VirtualAlloc failed: %d\n", error);
    }
    return result;
}

internal ALLOC_RELEASE_SIG(virtual_allocator_release) {
    if (VirtualFree(address, 0, MEM_RELEASE) == 0) {
        int error = GetLastError();
        fprintf(stderr, "VirtualFree failed: %d\n", error);
    }
}

internal ALLOC_COMMIT_SIG(malloc_allocator_commit) {
    void *result = nullptr;
    result = malloc(bytes);
    return result;
}

internal ALLOC_RELEASE_SIG(malloc_allocator_release) {
    Assert(address);
    free(address);
}

internal Base_Allocator *get_virtual_allocator() {
    local_persist Base_Allocator allocator = {};
    if (!allocator.commit_procedure) {
        // virtual_allocator.reserve_procedure = virtual
        allocator.commit_procedure  = virtual_allocator_commit;
        allocator.release_procedure = virtual_allocator_release;
    }
    return &allocator; 
}

internal Base_Allocator *get_malloc_allocator() {
    local_persist Base_Allocator allocator = {};
    if (!allocator.commit_procedure) {
        // allocator.reserve_procedure = malloc
        allocator.commit_procedure  =  malloc_allocator_commit;
        allocator.release_procedure = malloc_allocator_release;
    }
    return &allocator; 
}

internal Arena *arena_alloc(Base_Allocator *allocator, u64 size) {
    size += ARENA_HEADER_SIZE;
    // size = AlignForward(size, 8);
    void *memory = allocator->commit_procedure(size);
    MemoryZero(memory, size);
    Arena *arena = (Arena *)memory;
    if (arena) {
        arena->prev = nullptr;
        arena->current = arena;
        arena->pos = ARENA_HEADER_SIZE;
        arena->end = size;
        arena->base_pos = 0;
        arena->align = 8;
        arena->allocator = allocator;
    }
    return arena; 
}

internal Arena *make_arena(Base_Allocator *allocator) {
    Arena *arena = arena_alloc(allocator, KB(64));
    return arena;
}

internal void *arena_push(Arena *arena, u64 size) {
    Arena *current = arena->current;
    // printf("pos:%llu\n", current->pos);
    u64 new_pos = AlignForward(current->pos, arena->align);
    // printf("new_pos:%llu\n", new_pos);
    if (new_pos + size > current->end) {
        Arena *new_block = arena_alloc(arena->allocator, size);
        //@Todo Should this be aligned to power of 2?
        new_block->base_pos = (current->base_pos + current->end);
        new_block->prev = current;
        arena->current = new_block;
        new_pos = ARENA_HEADER_SIZE;
    }

    void *memory = (void *)((u8 *)arena->current + new_pos);
    MemoryZero(memory, size);
    arena->current->pos = new_pos + size;
    return memory;
}

internal void arena_release(Arena *arena) {
    for (Arena *node = arena, *prev = nullptr; node; node = prev) {
        prev = node->prev;
        arena->allocator->release_procedure(node);
    }
}

internal void arena_pop_to(Arena *arena, u64 pos) {
    pos = ClampBot(pos, ARENA_HEADER_SIZE);
    Arena *current = arena->current;
    while (current->base_pos > pos) {
        Arena *prev = current->prev;
        arena_release(current);
        current = prev;
    }
    Assert(current);
    current->pos = pos;
    arena->current = current;
}

internal void arena_clear(Arena *arena) {
    arena_pop_to(arena, 0);
}