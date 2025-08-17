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
    atlas_handler_t handler  = {};
    handler.zone             = zone;
    handler.atlas_width      = atlas_width;
    handler.atlas_height     = atlas_height;
    handler.atlas            = s_asset_create_texture_and_view(asset_manager, zone, atlas_width, atlas_height, BMF_RGBA32, false, TAAFT_NEAREST);
    handler.bitmap           = &handler.atlas.bitmap; 

    void *hash_table_memory  = c_za_alloc(zone, 1024 * sizeof(atlas_handler_hash_table_entry_t), ZA_TAG_TEXTURE);
    
    handler.contents         = c_hash_table_create(hash_table_memory, 1024, texture2D_t*);
    handler.textures_to_pack = c_dynamic_array_create(texture2D_t*, 64);
    handler.bitmap_cursor_x  = 0;
    handler.bitmap_cursor_y  = 0;
    handler.tallest_y        = 0;

    handler.is_initialized   = true;

    return(handler);
}

internal void
at_atlas_handler_add_texture(asset_manager_t *asset_manager, atlas_handler_t *handler, texture_view_t *view)
{
    Assert(handler->is_initialized   == true);
    Assert(handler->atlas_width      >  0);
    Assert(handler->atlas_height     >  0);
    Assert(handler->bitmap->data.data != null);

    atlas_handler_hash_table_entry_t entry_data;
    entry_data.texture_name =  view->asset_slot->filename;
    entry_data.texture_data = &view->asset_slot->texture;

    c_dynamic_array_append(&handler->textures_to_pack, &view->asset_slot->texture);
    c_hash_insert_kv_pair(&handler->contents, entry_data.texture_name, &entry_data);
}

internal void
at_atlas_handler_build_atlas(asset_manager_t *asset_manager, atlas_handler_t *handler, bool8 has_AA, filter_type_t filtering)
{
    Assert(handler->is_initialized    == true);
    Assert(handler->atlas_width        >  0);
    Assert(handler->atlas_height       >  0);
    Assert(handler->bitmap->data.data != null);

    if(handler->textures_to_pack.indices_used > 0)
    {
        r_make_gpu_texture(&handler->atlas, has_AA, filtering);
        string_t *bitmap_data = &handler->bitmap->data;

        for(u32 texture_index = 0;
            texture_index < handler->textures_to_pack.indices_used;
            ++texture_index)
        {
            texture2D_t *texture = c_dynamic_array_get(&handler->textures_to_pack, texture_index);
            Assert(texture);

            string_t *texture_data = &texture->bitmap.data;
            for(s32 y_index = 0;
                y_index < texture->bitmap.width;
                ++y_index)
            {
                for(s32 x_index = 0;
                    x_index < texture->bitmap.height;
                    ++x_index)
                {
                    u32 atlas_offset   = (handler->bitmap_cursor_y + y_index) * handler->atlas_width + (handler->bitmap_cursor_x + x_index) * handler->bitmap->channels;
                    u32 texture_offset = (y_index * texture->bitmap.width + x_index) * texture->bitmap.channels;
                    for(s32 channel_index = 0;
                        channel_index < texture->bitmap.channels;
                        ++channel_index)
                    {
                        bitmap_data->data[atlas_offset + channel_index] = texture_data->data[texture_offset + channel_index];
                    }
                }

                texture->view->GPU_textureID = handler->atlas.view->GPU_textureID;
                texture->uv_min        = vec2_create_float((float32)handler->bitmap_cursor_x,
                                                           (float32)handler->bitmap_cursor_y);
                texture->uv_max        = vec2_create_float((float32)handler->bitmap_cursor_x + texture->bitmap.width,
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
            }
        }

        c_dynamic_array_reset(&handler->textures_to_pack);
        r_update_texture_from_bitmap(asset_manager, &handler->atlas);
    }
    else
    {
        log_warning("Called to build an atlas, however there are no textures to be packed...\n");
    }
}
