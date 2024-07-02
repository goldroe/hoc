#include "core_arena.h"

internal Arena *arena_alloc(u64 size) {
    size = AlignForward(size, 2);
    void *memory = malloc(size);
    Arena *arena = (Arena *)memory;
    if (arena) {
        arena->prev = nullptr;
        arena->current = arena;
        arena->pos = ARENA_HEADER_SIZE;
        arena->end = size;
        arena->base_pos = 0;
        arena->align = 8;
    }
    return arena; 
}

internal Arena *arena_new() {
    Arena *arena = arena_alloc(MB(2));
    return arena;
}

internal void arena_free(Arena *arena) {
    free(arena);
}

internal void *arena_push(Arena *arena, u64 size) {
    Arena *current = arena->current;
    u64 new_pos = AlignForward(arena->pos, arena->align);
    if (new_pos + size > current->end) {
        Arena *new_block = arena_new();
        //@Todo Should this be aligned to power of 2?
        new_block->base_pos = (current->base_pos + current->end);
        new_block->prev = current;
        arena->current = new_block;
        current = new_block;
    }

    void *memory = (void *)((u8 *)arena->current + new_pos);
    MemoryZero(memory, size);
    arena->current->pos = new_pos + size;
    return memory;
}

internal void arena_pop_to(Arena *arena, u64 pos) {
    pos = ClampBot(pos, ARENA_HEADER_SIZE);
    Arena *current = arena->current;
    while (current->base_pos > pos) {
        Arena *prev = current->prev;
        arena_free(current);
        current = prev;
    }
    assert(current);
    current->pos = pos;
    arena->current = current;
}

internal void arena_clear(Arena *arena) {
    arena_pop_to(arena, 0);
}
