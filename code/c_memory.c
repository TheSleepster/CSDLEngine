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
c_arena_push_size(memory_arena_t *arena, u64 size_init)
{
    void *result = null;

    u8 *offset_ptr = ((u8*)arena->base + arena->used);
    u64 size = (size_init + 15) & ~15;
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

///////////////////
// ZONE ALLOCATOR
///////////////////

internal zone_allocator_t*
c_za_create(u64 block_size)
{
    zone_allocator_t *result = null;
    void *base          = os_allocate_memory(block_size + sizeof(zone_allocator_t));

    result              = base;
    result->base        = (u8*)base + sizeof(zone_allocator_t);
    result->capacity    = block_size;


    zone_allocator_block_t *block      = (zone_allocator_block_t *)(result->base);
    result->first_block.prev_block     = block;
    result->first_block.next_block     = result->first_block.prev_block;

    result->first_block.is_allocated   = true;
    result->first_block.allocation_tag = ZA_TAG_STATIC;
    result->first_block.block_id       = DEBUG_ZONE_ID;
    result->cursor                     = block;

    block->next_block             = &result->first_block;
    block->prev_block             =  block->next_block;
    block->next_block->prev_block =  block;
    block->is_allocated           =  false;
    block->block_size             =  result->capacity;
    block->block_id               =  DEBUG_ZONE_ID;

    return(result);
}

internal void
c_za_destroy(zone_allocator_t *zone)
{
    os_free_memory(zone, zone->capacity + sizeof(zone_allocator_t));
    zone = null;
}

internal void*
c_za_alloc(zone_allocator_t *zone, u64 size_init, za_allocation_tag_t tag)
{
    void *result = null;
    u64 size = (size_init + 15) & ~15;
    size     = size + sizeof(zone_allocator_block_t);

    zone_allocator_block_t *base_block = zone->cursor;
    if(!base_block->prev_block->is_allocated)
    {
        base_block = base_block->prev_block;
    }

    zone_allocator_block_t *block_cursor   = base_block;
    zone_allocator_block_t *starting_block = base_block;

    while(base_block->is_allocated || base_block->block_size < size)
    {
        if(block_cursor->is_allocated)
        {
            if(block_cursor->allocation_tag >= ZA_TAG_PURGELEVEL)
            {
                // NOTE(Sleepster): Free this block... 
                base_block = base_block->prev_block;
                c_za_free(zone, (byte *)block_cursor + sizeof(zone_allocator_block_t));

                base_block  = base_block->next_block;
                block_cursor = base_block->next_block;
            }
            else
            {
                // NOTE(Sleepster): This block cannot be freed, go next 
                block_cursor  = block_cursor->next_block;
                base_block   = block_cursor;
            }
        }
        else
        {
            block_cursor = block_cursor->next_block;
        }

        if(block_cursor == starting_block)
        {
            log_fatal("failed to allocate memory to the zone allocator... allocation size of: %d...\n", size);
            return(result);
        }
    }

    u64 leftover_memory = base_block->block_size - size;
    if(leftover_memory > MAX_MEMORY_FRAGMENTATION)
    {
        zone_allocator_block_t *new_block = (zone_allocator_block_t *)((byte*)base_block + size);
        new_block->block_size     = leftover_memory;
        new_block->is_allocated   = false;
        new_block->allocation_tag = ZA_TAG_NONE;
        new_block->prev_block     = base_block;
        new_block->next_block     = base_block->next_block;
        new_block->next_block->prev_block = new_block;
        new_block->block_id = 0;

        base_block->next_block = new_block;
        base_block->block_size = size;
    }

    base_block->is_allocated   = true;
    base_block->allocation_tag = tag;
    base_block->block_id       = DEBUG_ZONE_ID;
    zone->cursor               = base_block->next_block;

    result = (byte*)base_block + sizeof(zone_allocator_block_t);
    memset(result, 0, size);

    log_info("Zone Allocated: %d bytes...\n", size);
    return(result);
}

internal void
c_za_free(zone_allocator_t *zone, void *data)
{
    zone_allocator_block_t *block = null;
    zone_allocator_block_t *other = null;

    block = (zone_allocator_block_t *)((byte*)data - sizeof(zone_allocator_block_t));
    Assert(block->block_id == DEBUG_ZONE_ID);
    u64 block_size                = block->block_size;
    za_allocation_tag_t block_tag = block->allocation_tag;
    if(block->is_allocated)
    {
        block->is_allocated   = false;
        block->allocation_tag = ZA_TAG_NONE;
        block->block_id       = 0;
        
        other = block->prev_block;
        if(!other->is_allocated)
        {
            other->block_size += block->block_size;
            other->next_block  = block->next_block;
            other->next_block->prev_block = other;
            if(block == zone->cursor)
            {
                zone->cursor = other;
            }
            block = other;
        }

        other = block->next_block;
        if(!other->is_allocated)
        {
            block->block_size            += other->block_size;
            block->next_block             = other->next_block;
            block->next_block->prev_block = block;
            if(other == zone->cursor)
            {
                zone->cursor = block;
            }
        }
        data = null;
        log_info("Freed a zone block with a size of '%d'... had an allocation tag of '%d'...\n", block_size, block_tag);
    }
    else
    {
        log_error("Attempted to free a block in the zone allocator that has not been allocated...\n");
    }
}

internal void
c_za_DEBUG_print_block_list(zone_allocator_t *zone)
{
    zone_allocator_block_t *block = zone->first_block.next_block;
    log_info("Block List State:");
    while(block != &zone->first_block)
    {
        log_info("  Block at %u: size= %u, allocated= %u, tag= %d, id= %u",
            block, block->block_size, block->is_allocated, block->allocation_tag, block->block_id);
        block = block->next_block;
    }
    log_info("  Cursor at %u...", zone->cursor);
}

internal void
c_za_DEBUG_validate_block_list(zone_allocator_t *zone)
{
    zone_allocator_block_t *block = zone->first_block.next_block;
    if(block == &zone->first_block)
    {
        log_error("Zone Allocator block list is empty...");
    }

    for(;;)
    {
        Assert(block->prev_block->next_block == block);
        Assert(block->next_block->prev_block == block);

        block = block->next_block;
        if(block == &zone->first_block)
        {
            break;
        }
    }
    Assert(block == &zone->first_block);
}
