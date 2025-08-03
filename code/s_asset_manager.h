#if !defined(S_ASSET_MANAGER_H)
/* ========================================================================
   $File: s_asset_manager.h $
   $Date: Fri, 01 Aug 25: 11:57PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_ASSET_MANAGER_H
#include "c_types.h"
#include "c_debug.h"
#include "c_memory.h"
#include "c_file_api.h"
#include "c_hash_table.h"

#include "r_asset_shader.h"
#include "r_asset_texture.h"

typedef enum asset_type
{
    AT_NONE,
    AT_TEXTURE,
    AT_SHADER,
    AT_FONT,
    AT_SOUND,
    AT_ANIMATION,
    AT_COUNT
}asset_type_t;

typedef enum asset_slot_state
{
    ASS_INVALID,

    ASS_UNLOADED,
    ASS_QUEUED,
    ASS_LOADED,

    ASS_UNLOADING,

    ASS_COUNT
}asset_slot_state_t;

typedef struct asset_slot
{
    asset_slot_state_t slot_state;
    asset_type_t       asset_type;
    
    s32                asset_id;
    u64                last_used_ts;
    
    union
    {
        texture2D_t    texture;
        GPU_shader_t   shader;
        //loaded_sound_t sound;
        //dynamic_font_t render_font;
    };
}asset_slot_t;

typedef struct asset_manager
{
    u64 volatile      allocated_texture_memory;
    u64 volatile      allocated_shader_memory;
    u64 volatile      allocated_sound_memory;
    u64 volatile      allocated_font_memory;

    const u64         texture_memory_capacity;
    const u64         shader_memory_capacity;
    const u64         sound_memory_capacity;
    const u64         font_memory_capacity;
    
    zone_allocator_t *texture_allocator;
    zone_allocator_t *shader_allocator;
    zone_allocator_t *sound_allocator;
    zone_allocator_t *font_allocator;
    
    memory_arena_t   *trash_arena; // draw_frame_arena;
    memory_arena_t    manager_arena;

    hash_table_t      texture_hash;
    hash_table_t      shader_hash;
}asset_manager_t;

#endif
