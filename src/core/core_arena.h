#ifndef CORE_ARENA_H
#define CORE_ARENA_H

#define ARENA_HEADER_SIZE 128
struct Arena {
    Arena *prev;
    Arena *current;
    u64 pos;
    u64 end;
    u64 base_pos;
    int align;
};

internal Arena *arena_new();
internal void *arena_push(Arena *arena, u64 size);
internal void arena_clear(Arena *arena);
internal void arena_pop_to(Arena *arena, u64 pos);

#define push_array(arena, type, count) (type*)arena_push((arena), sizeof(type) * (count))

#endif // CORE_ARENA_H
