#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

arena_t arena;

arena_t
arena_init(size_t cap)
{
    assert(cap > 0);
    arena_t z;
    z.cap = cap;
    z.base = malloc(cap);
    assert(z.base);
    z.ptr = (uintptr_t) z.base;
    z.next_arena = NULL;
    return z;
}

void
arena_deinit(arena_t * ap)
{
    if (ap->next_arena) {
        arena_deinit(ap->next_arena);
    }
    free(ap->base);
}

void *
arena_alloc_align(arena_t * ap, size_t sz, size_t align)
{
    ap->ptr += (ap->ptr % align == 0) ? 0 : align - (ap->ptr % align);
    void * old_p = (void *) ap->ptr;
    if (ap->ptr + sz <= (uintptr_t) ap->base + ap->cap) {
        ap->ptr += sz;
        return old_p;
    }
    // this arena is full
    size_t new_cap = sz > (1 << 20) ? sz : (1 << 20);
    arena_t * new_arena = malloc(sizeof(*new_arena));
    *new_arena = *ap;
    *ap = arena_init(new_cap);
    ap->next_arena = new_arena;
    return arena_alloc_align(ap, sz, align);
}

void *
arena_alloc(arena_t * ap, size_t sz)
{
    size_t align = sz > 16 ? 16 : sz;
    return arena_alloc_align(ap, sz, align);
}

void *
arena_realloc_align(arena_t * ap, void * ptr, size_t old_sz, size_t new_sz, size_t align)
{
    void * new_p = arena_alloc_align(ap, new_sz, align);
    memcpy(new_p, ptr, old_sz);
    return new_p;
}

#if 0
// TODO: is it possible to free the most recent allocation?
static void *
arena_free(arena_t * ap, void * ptr)
{
}
#endif
