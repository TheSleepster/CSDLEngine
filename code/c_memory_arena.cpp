/* ========================================================================
   $File: c_memory_arena.cpp $
   $Date: Sun, 31 Aug 25: 10:43AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "c_memory_arena.h"

internal inline memory_arena_t
c_arena_create(u64 block_size)
{
    memory_arena_t result  = {};
    result.block_size      = block_size;
    result.capacity        = block_size - sizeof(memory_arena_footer_t);
    result.base            = (byte*)os_allocate_memory(block_size);
    result.scratch_counter = 0;
    result.used            = 0;
    result.is_initialized  = true;

    return(result);
}

internal inline memory_arena_footer_t*
c_arena_get_footer(memory_arena_t *arena)
{
    memory_arena_footer_t *result;
    result = (memory_arena_footer_t *)((u8*)arena->base + arena->capacity);

    return(result);
}

internal byte*
c_arena_push_size(memory_arena_t *arena, u64 size_init)
{
    Assert(arena->is_initialized == true);
    
    byte *result = null;

    u64 size = Align16(size_init);
    if((arena->used + size) >= arena->capacity)
    {
        if(arena->block_size == 0)
        {
            arena->block_size = MB(10);
        }

        memory_arena_footer_t footer;
        footer.last_base       = (u8 *)arena->base;
        footer.last_used       = arena->used;
        footer.last_capacity   = arena->capacity;
        footer.last_block_size = arena->block_size;

        size += sizeof(memory_arena_footer_t);
        u64 new_block_size = size > (arena->block_size + sizeof(memory_arena_footer_t)) ? size : arena->block_size;

        arena->capacity   = new_block_size - sizeof(memory_arena_footer_t);
        arena->base       = (byte *)os_allocate_memory(new_block_size);
        arena->used       = 0;
        arena->block_counter += 1;

        memory_arena_footer_t *arena_footer = c_arena_get_footer(arena);
        *arena_footer = footer;
    }
    Assert((arena->used + size) <= arena->capacity);
    Assert(arena->base != null);

    u8 *offset_ptr = ((u8*)arena->base + arena->used);
    result = offset_ptr;
    arena->used += size;

    return(result);
}

internal inline byte*
bootstrap_allocate_struct_(u64 struct_size, u64 offset_to_arena, u64 base_allocation)
{
    Assert(struct_size < base_allocation);

    byte *result = null;
    memory_arena_t bootstrap_arena = c_arena_create(base_allocation);
    result = c_arena_push_size(&bootstrap_arena, struct_size);

    *(memory_arena_t *)(result + offset_to_arena) = bootstrap_arena;
    return(result);
}

internal inline scratch_arena_t
c_begin_scratch_arena(memory_arena_t *arena)
{
    scratch_arena_t result;
    result.parent = arena;
    result.used   = arena->used;
    result.base   = (u8*)arena->base + arena->used;

    arena->scratch_counter += 1;

    return(result);
}

internal inline void
c_arena_free_last_block(memory_arena_t *arena)
{
    DEBUG_TIMED_BLOCK();

    u8 *block_to_free  = (u8*)arena->base;
    u64 old_block_size = arena->block_size;

    memory_arena_footer_t *footer = c_arena_get_footer(arena);
    arena->base       = footer->last_base;
    arena->used       = footer->last_used;
    arena->capacity   = footer->last_capacity;
    arena->block_size = footer->last_block_size;

    os_free_memory(block_to_free, old_block_size);
    arena->block_counter -= 1;
}

internal inline void
c_end_scratch_arena(scratch_arena_t *scratch_arena)
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
    DEBUG_TIMED_BLOCK();

    memset(arena->base, 0, arena->used);
    arena->used = 0;
}

internal inline void
c_arena_reset(memory_arena_t *arena)
{
    DEBUG_TIMED_BLOCK();

    while(arena->block_counter > 1)
    {
        c_arena_free_last_block(arena);
    }

    memset(arena->base, 0, arena->used);
    arena->used = 0;
}
