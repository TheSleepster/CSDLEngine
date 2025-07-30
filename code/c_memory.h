#if !defined(C_MEMORY_H)
/* ========================================================================
   $File: c_memory_arena.h $
   $Date: Wed, 02 Jul 25: 06:12PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_MEMORY_H
#include "c_base.h"
#include "c_types.h"
#include <stdlib.h>

internal void* os_allocate_memory(usize allocation_size);
internal void  os_free_memory(void *data, usize free_size);

/////////////////////////////////
// TODO: GET RID OF MALLOC
/////////////////////////////////

#define KB(x) ((x)     * 1024)
#define MB(x) (KB((x)) * 1024)
#define GB(x) (MB((x)) * 1024)

typedef struct memory_arena_footer
{
    u64 last_used;
    u8 *last_base;
    u64 last_capacity;
}memory_arena_footer_t;

typedef struct memory_arena
{
    void *base;
    u64   block_size;
    u64   capacity;
    u64   used;

    u32   block_counter;
    u32   scratch_counter;
}memory_arena_t;

typedef struct scratch_arena
{
    memory_arena_t *parent;
    u8             *base;
    u64             used;
}scratch_arena_t;

//////////// MEMORY ARENA API DEFINITIONS //////////////
internal inline memory_arena_t         c_arena_create(u64 block_size);
internal        void*                  c_arena_push_size(memory_arena_t *arena, u64 size);
internal inline scratch_arena_t        c_begin_scratch_arena(memory_arena_t *arena);
internal inline void                   c_end_scratch_arena(scratch_arena_t *scratch);
internal inline memory_arena_footer_t* c_arena_get_footer(memory_arena_t *arena);
internal inline void                   c_arena_free_last_block(memory_arena_t *arena);
internal inline void                   c_arena_clear_block(memory_arena_t *arena);
internal inline void                   c_arena_reset(memory_arena_t *arena);

// MACROS
#define c_arena_push_struct(arena, type)                 (type*)c_arena_push_size(arena, sizeof(type))
#define c_arena_push_array(arena, type, count)           (type*)c_arena_push_size(arena, (sizeof(type)) * count)
#define c_bootstrap_allocate_struct(type, member, alloc) (type*)_bootstrap_allocate_struct(sizeof(type), OffsetOf(type, member), alloc)
////////////////////////////////////////////////////////

typedef enum za_allocation_tag
{
    NONE       = 0,
    DYNAMIC    = 1,
    CACHED     = 2,

    UNPURGABLE = 50,
    STATIC     = 100
}za_allocation_tag_t;

typedef struct zone_allocator_block
{
    u32                          block_id;
    bool8                        is_used;
    u64                          block_size;
    u64                          allocation_tag;

    struct zone_allocator_block *next_block;
    struct zone_allocator_block *prev_block;
}zone_allocator_block_t;

typedef struct zone_allocator
{
    u64                     capacity;
    u8                     *base;

    zone_allocator_block_t  first_block;
    zone_allocator_block_t *cursor;
}zone_allocator_t;

//////////// ZONE ALLOCATOR API DEFINITIONS /////////////
// TODO(Sleepster): create the functions 
/////////////////////////////////////////////////////////

#endif
