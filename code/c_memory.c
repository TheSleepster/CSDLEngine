/* ========================================================================
   $File: c_memory.c $
   $Date: Wed, 23 Jul 25: 12:57PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "c_memory.h"

internal inline memory_arena_t
c_arena_create(u64 block_size)
{
    memory_arena_t result  = {};
    result.capacity        = block_size;
    result.block_size      = block_size;
    result.scratch_counter = 0;
    result.used            = 0;
    result.base            = os_allocate_memory(block_size);

    return(result);
}

internal inline memory_arena_footer_t*
c_arena_get_footer(memory_arena_t *arena)
{
    memory_arena_footer_t *result;
    result = (memory_arena_footer_t *)(arena->base + arena->capacity);

    return(result);
}

internal void*
c_arena_push_size(memory_arena_t *arena, u64 size)
{
    void *result = null;

    u8 *offset_ptr = ((u8*)arena->base + arena->used);
    if((arena->used + size) >= arena->capacity)
    {
        if(arena->block_size == 0)
        {
            arena->block_size = MB(10);
        }

        memory_arena_footer_t footer;
        footer.last_base = arena->base;
        footer.last_used = arena->used;
        footer.last_capacity = arena->capacity;

        u64 new_block_size = size > (arena->block_size + sizeof(memory_arena_footer_t)) ? size : arena->block_size;

        arena->capacity = new_block_size - sizeof(memory_arena_footer_t);
        arena->base     = os_allocate_memory(new_block_size);
        arena->used     = 0;
        arena->block_counter += 1;

        memory_arena_footer_t *arena_footer = c_arena_get_footer(arena);
        *arena_footer = footer;
    }
    Assert((arena->used + size) <= arena->capacity);
    Assert(arena->base != null);

    result       = offset_ptr;
    arena->used += size;

    return(result);
}

internal inline void*
_bootstrap_allocate_struct(u64 struct_size, u64 offset_to_arena, u64 base_allocation)
{
    Assert(struct_size > base_allocation);
    
    memory_arena_t bootstrap_arena = c_arena_create(base_allocation);
    bootstrap_arena.base          += struct_size;
    bootstrap_arena.capacity      -= struct_size;

    *(memory_arena_t *)((u8*)bootstrap_arena.base + offset_to_arena) = bootstrap_arena;
    return((void*)bootstrap_arena.base);
}

internal inline scratch_arena_t
c_begin_scratch_arena(memory_arena_t *arena)
{
    scratch_arena_t result;
    result.parent = arena;
    result.used   = arena->used;
    result.base   = arena->base + arena->used;

    arena->scratch_counter += 1;

    return(result);
}

internal inline void
c_arena_free_last_block(memory_arena_t *arena)
{
    u8 *block_to_free = arena->base;
    u64 old_capacity  = arena->capacity;

    memory_arena_footer_t *footer = c_arena_get_footer(arena);
    arena->base     = footer->last_base;
    arena->used     = footer->last_used;
    arena->capacity = footer->last_capacity;

    os_free_memory(block_to_free, old_capacity);
    arena->block_counter -= 1;
}

internal inline void
c_end_scrach_arena(scratch_arena_t *scratch_arena)
{
    memory_arena_t *parent = scratch_arena->parent;
    Assert(parent->scratch_counter > 0);
    while(parent->base != scratch_arena->base)
    {
        c_arena_free_last_block(parent);
    }
    Assert(parent->used >= scratch_arena->used);
    
    parent->used = scratch_arena->used;
    parent->base = scratch_arena->base;

    parent->scratch_counter -= 1;
}

internal inline void
c_arena_clear_block(memory_arena_t *arena)
{
    memset(arena->base, 0, arena->used);
    arena->used = 0;
}

internal inline void
c_arena_reset(memory_arena_t *arena)
{
    while(arena->block_counter > 1)
    {
        c_arena_free_last_block(arena);
    }

    memset(arena->base, 0, arena->used);
    arena->used = 0;
}
