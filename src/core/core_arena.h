#ifndef CORE_ARENA_H
#define CORE_ARENA_H

#define ALLOC_RESERVE_SIG(name) void * name(u64 bytes)
typedef ALLOC_RESERVE_SIG(Alloc_Reserve);

#define ALLOC_COMMIT_SIG(name) void * name(u64 bytes)
typedef ALLOC_COMMIT_SIG(Alloc_Commit);

#define ALLOC_RELEASE_SIG(name) void name(void *address)
typedef ALLOC_RELEASE_SIG(Alloc_Release);

struct Core_Allocator {
    Alloc_Reserve *reserve_procedure;
    Alloc_Commit *commit_procedure;
    Alloc_Release *release_procedure;
};

#define ARENA_HEADER_SIZE 128
struct Arena {
    Arena *prev;
    Arena *current;
    u64 pos;
    u64 end;
    u64 base_pos;
    int align;
    Core_Allocator *allocator;
};

internal Arena *make_arena(Core_Allocator *allocator);
internal void *arena_push(Arena *arena, u64 size);
internal void arena_clear(Arena *arena);
internal void arena_pop_to(Arena *arena, u64 pos);

#define push_array(arena, type, count) (type*)arena_push((arena), sizeof(type) * (count))

#endif // CORE_ARENA_H
