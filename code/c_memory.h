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

#define KB(x) ((u64)(x) * 1024ULL)
#define MB(x) (KB((x))  * 1024ULL)
#define GB(x) (MB((x))  * 1024ULL)

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

    bool8 is_initialized;

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
#define DEBUG_ZONE_ID            0x1d4a11
#define MAX_MEMORY_FRAGMENTATION 64

typedef enum za_allocation_tag
{
    ZA_TAG_NONE       = 0,
    ZA_TAG_STATIC     = 1, // Entire runtime
    ZA_TAG_TEXTURE    = 2,
    ZA_TAG_SOUND      = 3,
    ZA_TAG_FONT       = 4,

    // >= 100 are purgeable when needed
    ZA_TAG_PURGELEVEL = 100,
    ZA_TAG_CACHE      = 101,
}za_allocation_tag_t;

typedef struct zone_allocator_block
{
    u32                          block_id;
    bool8                        is_allocated;
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
#define c_za_push_struct(zone, type, tag)        c_za_alloc(zone, sizeof(type), tag);
#define c_za_push_array(zone, type, count, tag)  c_za_alloc(zone, sizeof(type) * count, tag);

internal zone_allocator_t* c_za_create(u64 block_size);
internal void              c_za_destroy(zone_allocator_t *zone);
internal void*             c_za_alloc(zone_allocator_t *zone, u64 size_init, za_allocation_tag_t tag);
internal void              c_za_free(zone_allocator_t  *zone, void *data);
internal void              c_za_free_tag(zone_allocator_t *zone, za_allocation_tag_t tag);

// DEBUG FUNCTIONS
internal void c_za_DEBUG_print_block_list(zone_allocator_t *zone);
internal void c_za_DEBUG_validate_block_list(zone_allocator_t *zone);
/////////////////////////////////////////////////////////

#endif
