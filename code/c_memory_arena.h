#if !defined(C_MEMORY_ARENA_H)
/* ========================================================================
   $File: c_memory_arena.h $
   $Date: Sun, 31 Aug 25: 10:43AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_MEMORY_ARENA_H
#include "c_base.h"
#include "c_types.h"
#include <stdlib.h>

#define KB(x) ((u64)(x) * 1024ULL)
#define MB(x) (KB((x))  * 1024ULL)
#define GB(x) (MB((x))  * 1024ULL)

/*===========================================
  ============ MEMORY ARENA API  ============
  ===========================================*/
typedef struct memory_arena_footer
{
    u64 last_used;
    u8 *last_base;
    u64 last_capacity;
    u64 last_block_size;
}memory_arena_footer_t;

typedef struct memory_arena
{
    byte *base;
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
internal        byte*                  c_arena_push_size_(memory_arena_t *arena, u64 size);
internal inline scratch_arena_t        c_begin_scratch_arena(memory_arena_t *arena);
internal inline void                   c_end_scratch_arena(scratch_arena_t *scratch);
internal inline memory_arena_footer_t* c_arena_get_footer(memory_arena_t *arena);
internal inline void                   c_arena_free_last_block(memory_arena_t *arena);
internal inline void                   c_arena_clear_block(memory_arena_t *arena);
internal inline void                   c_arena_reset(memory_arena_t *arena);
internal inline byte*                  bootstrap_allocate_struct_(u64 struct_size, u64 offset_to_arena, u64 base_allocation);

// MACROS
#if MEMORY_DEBUGGING 
    #define c_arena_push_size(arena, size)                                          c_arena_push_size_(arena, size);                                                               log_info("Arena Allocation of: '%ull' bytes from file: '%s', line: '%d'...\n", size, __FILE__, __LINE__)
    #define c_arena_push_struct(arena, type)                                 (type*)c_arena_push_size_(arena,   sizeof(type));                                                     log_info("Arena Allocation of: '%ull' bytes from file: '%s', line: '%d'...\n", sizeof(type), __FILE__, __LINE__)
    #define c_arena_push_array(arena, type, count)                           (type*)c_arena_push_size_(arena,  (sizeof(type)) * count);                                            log_info("Arena Allocation of: '%ull' bytes from file: '%s', line: '%d'...\n", sizeof(type) * count, __FILE__, __LINE__)
    #define c_arena_bootstrap_allocate_struct(type, member, allocation_size) (type*)bootstrap_allocate_struct_(sizeof(type), IntFromPtr(OffsetOf(type, member)), allocation_size); log_info("Arena Allocation of: '%ull' bytes from file: '%s', line: '%d'...\n", allocation_size, __FILE__, __LINE__)
#else
    #define c_arena_push_size(arena, size)                                          c_arena_push_size_(arena, size) 
    #define c_arena_push_struct(arena, type)                                 (type*)c_arena_push_size_(arena,   sizeof(type))
    #define c_arena_push_array(arena, type, count)                           (type*)c_arena_push_size_(arena,  (sizeof(type)) * count)
    #define c_arena_bootstrap_allocate_struct(type, member, allocation_size) (type*)bootstrap_allocate_struct_(sizeof(type), IntFromPtr(OffsetOf(type, member)), allocation_size)
#endif
////////////////////////////////////////////////////////

#endif
