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
#include "r_asset_dynamic_render_font.h"
#include "r_asset_texture.h"

#define MANAGER_HASH_TABLE_SIZE 1024
typedef enum asset_type
{
    AT_NONE,
    AT_BITMAP,
    AT_SHADER,
    AT_FONT,
    AT_SOUND,
    AT_ANIMATION,
    AT_COUNT
}asset_type_t;

// NOTE(Sleepster): Unfortunate naming 
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

    string_t           filename;
    
    s32                asset_id;
    s32                asset_file_id;
    u64                last_used_ts;

    u64                asset_file_data_offset;
    u64                asset_file_data_length;
    union
    {
        texture2D_t           texture;
        GPU_shader_t          shader;
        //loaded_sound_t        sound;
        dynamic_render_font_t render_font;
    };
}asset_slot_t;

/* NOTE(Sleepster): New idea: asset_handles. These asset_views will be
 * used to look into an asset and keep track of the state related to
 * that asset while still allowing the asset's state to remain
 * modifiable. The idea is that we no longer hand out pointers to the
 * texture like before, we instead create "handles" that contains the
 * data needed to actually use the texture but in a read only
 * way. Such that if important data like the texture's UVs change due
 * to the engine merging certain textures into an atlas, we won't have
 * to update the handles. The handles will already contain the right
 * data without updating.
 */

typedef struct asset_handle
{
    bool8        is_valid;
    asset_type_t type;
    union
    {
        const texture_view_t *texture;
    };
}asset_handle_t;

typedef struct asset_manager
{
    u64 volatile      allocated_texture_memory;
    u64 volatile      allocated_shader_memory;
    u64 volatile      allocated_sound_memory;
    u64 volatile      allocated_font_memory;

    // NOTE(Sleepster): READ ONLY... DO NOT MODIFY THESE BEYOND THIS INIT FUNCTION
    u64               texture_memory_capacity;
    u64               shader_memory_capacity;
    u64               sound_memory_capacity;
    u64               font_memory_capacity;
    
    zone_allocator_t *texture_allocator;
    zone_allocator_t *shader_allocator;
    zone_allocator_t *sound_allocator;
    zone_allocator_t *font_allocator;
    
    memory_arena_t   *trash_arena;  // draw_frame_arena;
    memory_arena_t    manager_arena;

    // TODO(Sleepster): read in portions of the file, not the whole thing...
    // What are we supposed to do with this? What if we have more than one asset file?
    // FILE HANDLE???
    string_t          asset_file_data;
    file_t            asset_file_handle;

    hash_table_t      texture_hash;
    hash_table_t      shader_hash;
    hash_table_t      font_hash;
    hash_table_t      sound_hash;

    u32               texture_view_count;
    u32               font_view_count;
    u32               shader_view_count;

    array_t           texture_views;
    array_t           font_views;
    array_t           shader_views;

    texture_view_t    null_texture;
    texture_view_t    null_font;
    texture_view_t    null_shader;
}asset_manager_t;

#endif
