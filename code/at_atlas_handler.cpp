/* ========================================================================
   $File: at_atlas_handler.c $
   $Date: Thu, 14 Aug 25: 07:43PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "at_atlas_handler.h"

internal atlas_handler_t
at_atlas_handler_create(asset_manager_t  *asset_manager,
                        zone_allocator_t *zone,
                        s32               atlas_width,
                        s32               atlas_height)
{
    atlas_handler_t handler       = {};
    handler.zone                  = zone;
    handler.atlas_width           = atlas_width;
    handler.atlas_height          = atlas_height;
    handler.atlas                 = s_asset_texture_and_view_create(asset_manager, zone, atlas_width, atlas_height, BMF_RGBA32, false, TAAFT_NEAREST);
    handler.bitmap                = &handler.atlas.bitmap; 
    handler.atlas.bitmap.channels = 4;

    asset_manager->gpu_data->r_texture_make_gpu(&handler.atlas, false, TAAFT_NEAREST);
    void *hash_table_memory  = c_za_alloc(zone, 1024 * sizeof(atlas_handler_hash_table_entry_t), ZA_TAG_TEXTURE);
    
    handler.contents           = c_hash_table_create(hash_table_memory, 1024, atlas_handler_hash_table_entry_t*);
    handler.textures_to_pack   = c_dynamic_array_create(asset_handle_t, 64);
    handler.textures_to_update = c_dynamic_array_create(asset_handle_t, 64);
    handler.bitmap_cursor_x    = 0;
    handler.bitmap_cursor_y    = 0;
    handler.tallest_y          = 0;
    handler.is_initialized     = true;

    return(handler);
}

internal void
at_atlas_handler_update_entry(asset_manager_t *asset_manager, atlas_handler_t *handler, asset_handle_t handle)
{
    DEBUG_TIMED_BLOCK();

    Assert(handler->is_initialized  == true);
    Assert(handle.texture           != null);
    Assert(handle.asset_slot        != null);
    Assert(handle.texture->uv_min   != null);
    Assert(handle.texture->uv_max   != null);

    u32 x_max = floorf(handle.texture->uv_max->x);
    u32 y_max = floorf(handle.texture->uv_max->y);
    if(x_max > 0 && y_max > 0)
    {
        c_dynamic_array_append_value(&handler->textures_to_update, handle);
    }
}

internal void
at_atlas_handler_add_texture(asset_manager_t *asset_manager, atlas_handler_t *handler, asset_handle_t handle)
{
    DEBUG_TIMED_BLOCK();

    Assert(handler->is_initialized           == true);
    Assert(handler->atlas_width               >  0);
    Assert(handler->atlas_height              >  0);
    Assert(handler->atlas.bitmap.data.data   != null);
    Assert(handle.texture                    != null);
    Assert(handle.asset_slot                 != null);
    Assert(handle.asset_slot->filename.data  != null);
    if(!handle.texture->is_in_atlas)
    {
        atlas_handler_hash_table_entry_t *entry = (atlas_handler_hash_table_entry_t *)c_hash_get_value(&handler->contents, handle.asset_slot->filename);
        if(!entry)
        {
            atlas_handler_hash_table_entry_t entry_data;
            entry_data.texture_name =  handle.asset_slot->filename;
            entry_data.texture_data = &handle.asset_slot->texture;

            c_dynamic_array_append_value(&handler->textures_to_pack, handle);
            c_hash_insert_kv_pair(&handler->contents, entry_data.texture_name, &entry_data);

            handle.texture->is_in_atlas = true;
        }
    }
}

internal void
at_atlas_handler_build_atlas(asset_manager_t *asset_manager, atlas_handler_t *handler)
{
    DEBUG_TIMED_BLOCK();

    Assert(handler->is_initialized     == true);
    Assert(handler->atlas_width         >  0);
    Assert(handler->atlas_height        >  0);
    Assert(handler->atlas.bitmap.data.count >=  0);

    bool8 is_dirty = false;

    string_t *bitmap_data = &handler->atlas.bitmap.data;
    if(handler->textures_to_pack.indices_used > 0)
    {
        for(u32 texture_index = 0;
            texture_index < handler->textures_to_pack.indices_used;
            ++texture_index)
        {
            asset_handle_t *asset_handle = (asset_handle_t*)c_dynamic_array_get(&handler->textures_to_pack, texture_index);
            texture2D_t    *texture      = &asset_handle->asset_slot->texture;
            Assert(texture);

            string_t *texture_data = &texture->bitmap.decompressed_data;
            for(s32 y_index = 0;
                y_index < texture->bitmap.height;
                ++y_index)
            {
                for(s32 x_index = 0;
                    x_index < texture->bitmap.width;
                    ++x_index)
                {
                    u32 atlas_offset = ((handler->bitmap_cursor_y + y_index) * handler->atlas_width +
                                        (handler->bitmap_cursor_x + x_index)) * handler->atlas.bitmap.channels;

                    u32 texture_offset = (y_index * texture->bitmap.width + x_index) * texture->bitmap.channels;

                    for(s32 channel_index = 0;
                        channel_index < texture->bitmap.channels;
                        ++channel_index)
                    {
                        bitmap_data->data[atlas_offset + channel_index] =
                            texture_data->data[texture_offset + channel_index];
                    }
                }
            }

            asset_manager->gpu_data->r_texture_delete(texture->view);
            texture->view->GPU_textureID = handler->atlas.view->GPU_textureID;
            texture->uv_min = vec2_create_float((float32)handler->bitmap_cursor_x,
                                                (float32)handler->bitmap_cursor_y);
            texture->uv_max = vec2_create_float((float32)handler->bitmap_cursor_x + texture->bitmap.width,
                                                (float32)handler->bitmap_cursor_y + texture->bitmap.height);
            if(texture->bitmap.height > handler->tallest_y)
            {
                handler->tallest_y = texture->bitmap.height;
            }

            handler->bitmap_cursor_x += texture->bitmap.width;
            handler->bitmap_cursor_y += texture->bitmap.height;
            if(handler->bitmap_cursor_x >= handler->atlas_width)
            {
                handler->bitmap_cursor_x  = 0;
                handler->bitmap_cursor_y += handler->tallest_y;
            }

            is_dirty = true;
            s_asset_texture_destroy_data(asset_manager, *asset_handle);
        }
        c_dynamic_array_reset(&handler->textures_to_pack);
    }

    if(handler->textures_to_update.indices_used > 0)
    {
        for(u32 texture_index = 0;
            texture_index < handler->textures_to_update.indices_used;
            ++texture_index)
        {
            asset_handle_t *asset_handle = (asset_handle_t*)c_dynamic_array_get(&handler->textures_to_update, texture_index);
            texture2D_t    *texture      = &asset_handle->asset_slot->texture;
            Assert(texture);

            string_t       *texture_data = &texture->bitmap.decompressed_data;
            if(texture_data->data)
            {
                texture_view_t *texture_view = asset_handle->texture;

                for(s32 y_index = 0;
                    y_index < texture->bitmap.height;
                    ++y_index)
                {
                    for(s32 x_index = 0;
                        x_index < texture->bitmap.width;
                        ++x_index)
                    {
                        u32 atlas_offset   = ((texture_view->uv_min->y + y_index) * handler->atlas_width +
                                              (texture_view->uv_min->x + x_index)) * handler->atlas.bitmap.channels;

                        u32 texture_offset = (y_index * texture->bitmap.width + x_index) * texture->bitmap.channels;
                        for(s32 channel_index = 0;
                            channel_index < texture->bitmap.channels;
                            ++channel_index)
                        {
                            bitmap_data->data[atlas_offset + channel_index] =
                                texture_data->data[texture_offset + channel_index];
                        }
                    }
                }

                log_trace("Texture: '%s' has been updated in the atlas...\n", asset_handle->asset_slot->filename.data);
                is_dirty = true;
                s_asset_texture_destroy_data(asset_manager, *asset_handle);
            }
        }
        c_dynamic_array_reset(&handler->textures_to_update);
    }

    if(is_dirty)
    {
        asset_manager->gpu_data->r_texture_update_from_bitmap(asset_manager, &handler->atlas);
        log_trace("Updated atlas...\n");
    }
}
