#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct arena_t arena_t;

struct arena_t {
    void * base;
    size_t cap;
    uintptr_t ptr;
    arena_t * next_arena;
};

extern arena_t arena;

arena_t arena_init(size_t cap);
void    arena_deinit(arena_t * zp);
void *  arena_alloc_align(arena_t * ap, size_t sz, size_t align);
void *  arena_alloc(arena_t * ap, size_t sz);
void *  arena_realloc_align(arena_t * ap, void * ptr, size_t old_sz, size_t new_sz, size_t align);

#endif /* ARENA_H */
