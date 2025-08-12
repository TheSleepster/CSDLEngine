/* ========================================================================
   $File: r_asset_texture.c $
   $Date: Sat, 02 Aug 25: 01:05AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "s_asset_manager.h"
#include "r_asset_texture.h"

#include "c_hash_table.h"

internal asset_handle_t
s_asset_get_texture_handle(asset_manager_t *asset_manager, string_t asset_key)
{
    asset_handle_t result = {};

    asset_slot_t *valid_slot = c_hash_get_value(&asset_manager->texture_hash, asset_key);
    result.type = AT_BITMAP;
    if(valid_slot)
    {
        result.is_valid = true;
        result.type     = AT_BITMAP;

        texture2D_t *texture_data = &valid_slot->texture;
        if(texture_data->view)
        {
            // valid view
            result.texture = texture_data->view;
        }
        else
        {
            // generate a new view 
            texture_view_t *new_view = c_array_get_value(&asset_manager->texture_views, asset_manager->texture_view_count);
            new_view->viewID         = asset_manager->texture_view_count;
            new_view->GPU_textureID  = 0; 
            new_view->uv_min         = &texture_data->uv_min;
            new_view->uv_max         = &texture_data->uv_max;
            new_view->asset_slot     = valid_slot;

            asset_manager->texture_view_count++;

            texture_data->view = new_view;
            result.texture     = new_view;
        }
    }
    else
    {
        log_warning("Invalid texture key: '%s', could not find a texture with that name in our packaging system...\n", asset_key.data);
        result.is_valid = false;
        result.texture  = &asset_manager->null_texture;
    }

    return(result);
}

internal void
s_asset_load_texture_data(asset_manager_t *asset_manager, asset_handle_t handle)
{
    asset_slot_t *slot_data = handle.texture->asset_slot;
    Assert(slot_data->slot_state != ASS_LOADED);
    
    if(slot_data->asset_file_data_offset > 0)
    {
        slot_data->texture.bitmap.data = c_file_read_za(asset_manager->texture_allocator,
                                                        asset_manager->asset_file_handle.filepath,
                                                        slot_data->asset_file_data_length,
                                                        slot_data->asset_file_data_offset,
                                                        ZA_TAG_TEXTURE);
    }
    else
    {
        // NOTE(Sleepster): Data wasn't in the asset file, load it from the filepath
        Assert(slot_data->filename.data != null);
        slot_data->texture.bitmap.data = c_file_read_za(asset_manager->texture_allocator,
                                                        slot_data->filename,
                                                        slot_data->asset_file_data_length,
                                                        slot_data->asset_file_data_offset,
                                                        ZA_TAG_TEXTURE);
    }
    slot_data->texture.bitmap.data.data = stbi_load_from_memory(slot_data->texture.bitmap.data.data,
                                                                slot_data->texture.bitmap.data.count,
                                                               &slot_data->texture.bitmap.width,
                                                               &slot_data->texture.bitmap.height,
                                                               &slot_data->texture.bitmap.channels,
                                                                BMF_RGBA32);

    slot_data->slot_state            = ASS_LOADED;
    slot_data->texture.bitmap.format = BMF_RGBA32;
    slot_data->texture.bitmap.stride = 32;

    r_make_gpu_texture(&slot_data->texture, false, TAAFT_NEAREST);
}
