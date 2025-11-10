#if !defined(AT_ATLAS_HANDLER_H)
/* ========================================================================
   $File: at_atlas_handler.h $
   $Date: Thu, 14 Aug 25: 07:43PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define AT_ATLAS_HANDLER_H
#include "c_base.h"
#include "c_types.h"
#include "c_log_assert.h"
#include "c_memory_arena.h"
#include "c_zone_allocator.h"
#include "c_string.h"
#include "c_array.h"
#include "c_hash_table.h"

#include "r_asset_texture.h"

#define ENGINE_ATLAS_SIZE 4096

typedef struct asset_manager asset_manager_t;

typedef struct atlas_handler_hash_table_entry
{
    string_t     texture_name;
    texture2D_t *texture_data;
}atlas_handler_hash_table_entry_t;

typedef struct atlas_handler
{
    bool8             is_initialized;
    zone_allocator_t *zone;
    
    s32               atlas_width;
    s32               atlas_height;
    
    texture2D_t       atlas;
    bitmap_t         *bitmap;

    hash_table_t      contents;

    dynamic_array_t   textures_to_pack;
    dynamic_array_t   textures_to_update;

    s32               bitmap_cursor_x;
    s32               bitmap_cursor_y;
    s32               tallest_y;
}atlas_handler_t;

////////////////////
// API DEFINITIONS
////////////////////
internal atlas_handler_t at_atlas_handler_create(asset_manager_t  *asset_manager, zone_allocator_t *zone, s32 atlas_width, s32 atlas_height);
internal void            at_atlas_handler_add_texture(asset_manager_t *asset_manager, atlas_handler_t *handler, asset_handle_t handle);
internal void            at_atlas_handler_update_entry(asset_manager_t *asset_manager, atlas_handler_t *handler, asset_handle_t handle);
internal void            at_atlas_handler_build_atlas(asset_manager_t *asset_manager, atlas_handler_t *handler);
////////////////////

#endif
