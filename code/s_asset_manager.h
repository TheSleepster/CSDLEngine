#if !defined(S_ASSET_MANAGER_H)
/* ========================================================================
   $File: s_asset_manager.h $
   $Date: Fri, 01 Aug 25: 11:57PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_ASSET_MANAGER_H
#include "c_types.h"
#include "c_log_assert.h"
#include "c_memory_arena.h"
#include "c_file_api.h"
#include "c_hash_table.h"

#include "r_asset_shader.h"
#include "r_asset_dynamic_render_font.h"
#include "r_asset_texture.h"
#include "at_atlas_handler.h"
#include "a_asset_loaded_sound.h"

#define MANAGER_HASH_TABLE_SIZE 1024
#define MAX_TEXTURE_VIEWS       1024

typedef struct asset_file_table_of_contents asset_file_table_of_contents_t;

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

    ASS_RELOADING,
    ASS_UNLOADING,

    ASS_COUNT
}asset_slot_state_t;

typedef struct asset_slot
{
    asset_slot_state_t slot_state;
    asset_type_t       asset_type;

    string_t           filename;
    string_t           system_filepath;
    
    s32                asset_id;
    s32                asset_file_id;
    u64                last_used_ts;

    u64                asset_file_data_offset;
    u64                asset_file_data_length;
    union
    {
        texture2D_t           texture;
        GPU_shader_t          shader;
        loaded_sound_t        loaded_sound;
        dynamic_render_font_t render_font;
    };
}asset_slot_t;

/* NOTE(Sleepster): New idea: asset_handles. These asset_handles will be
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
    bool8         is_valid;
    asset_type_t  type;

    asset_slot_t *asset_slot;
    union
    {
        texture_view_t        *texture;
        dynamic_render_font_t *font;
        loaded_sound_t        *sound;
    };
}asset_handle_t;

typedef struct playing_sound
{
    asset_handle_t        sound_handle;
    asset_handle_t        next_sound_handle;

    float32               play_cursor;
    vec2_t                current_playing_volume;
    vec2_t                target_playing_volume;
    vec2_t                d_volumet;
    float32               pitch_shift; 

    bool8                 is_paused;
    struct playing_sound *next;
}playing_sound_t;

typedef struct asset_manager
{
    multithreading_work_queue_manager_t *queue_manager;
    
    // NOTE(Sleepster): READ ONLY... DO NOT MODIFY THESE BEYOND THE INIT FUNCTION
    u64               texture_memory_capacity;
    u64               shader_memory_capacity;
    u64               sound_memory_capacity;
    u64               font_memory_capacity;
    
    memory_arena_t   *trash_arena;  // draw_frame_arena;
    memory_arena_t    manager_arena;

    // TODO(Sleepster): What are we supposed to do with this? What if we have more than one asset file?
    string_t          asset_file_data;
    file_t            asset_file_handle;

    struct 
    {
        zone_allocator_t *texture_allocator;
        atlas_handler_t   primary_handler;

        hash_table_t      texture_hash;
        array_t           texture_views;
        u32               texture_view_count;

        texture_view_t    null_texture;
    }texture_catalog;
    struct 
    {
        zone_allocator_t *shader_allocator;

        hash_table_t      shader_hash;
        array_t           shader_views;
        u32               shader_view_count;

        texture_view_t    null_shader;
    }shader_catalog;
    struct 
    {
        FT_Library        font_lib;
        zone_allocator_t *font_allocator;
    
        hash_table_t      font_hash;
        array_t           font_views;
        u32               font_view_count;

        texture_view_t    null_font;
    }font_catalog;
    struct
    {
        zone_allocator_t *sound_allocator;

        hash_table_t      sound_hash;
        array_t           sound_views;
        u32               sound_view_count;

        texture_view_t    null_sound;
    }sound_catalog;
}asset_manager_t;

internal void     s_asset_manager_read_asset_file_entries(asset_manager_t *asset_manager, string_t entry_data, asset_file_table_of_contents_t *table_of_contents);
internal void     s_asset_manager_init(asset_manager_t *asset_manager, string_t packed_asset_filepath);
internal void     s_asset_load_data_from_asset_file_or_path(asset_manager_t *asset_manager, string_t *out_data, zone_allocator_t *zone, asset_slot_t *slot_data, za_allocation_tag_t tag, bool8 is_reloading);

#endif
