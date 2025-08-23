/* ========================================================================
   $File: r_asset_texture.c $
   $Date: Sat, 02 Aug 25: 01:05AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "s_asset_manager.h"
#include "r_asset_texture.h"

#include "c_hash_table.h"

internal bitmap_t
s_asset_bitmap_create(zone_allocator_t *zone, s32 width, s32 height, bitmap_format_t format)
{
    bitmap_t result   = {};
    result.width      = width;
    result.height     = height;
    result.channels   = format;
    result.stride     = 8 * format;
    result.format     = format;
    result.data.count = width * height * format;
    result.data.data  = c_za_alloc(zone, result.data.count, ZA_TAG_TEXTURE);

    return(result);
}

internal texture_view_t *
s_asset_texture_view_generate(asset_manager_t *asset_manager, asset_slot_t *valid_slot, texture2D_t *texture_data)
{
    texture_view_t *new_view = 0; 
    for(u32 view_index = 0;
        view_index < asset_manager->texture_catalog.texture_views.capacity;
        ++view_index)
    {
        texture_view_t *found = (texture_view_t *)c_array_get_value(&asset_manager->texture_catalog.texture_views, view_index);
        if(!found->is_valid)
        {
            new_view = found;
            break;
        }
    }
    
    if(new_view)
    {
        new_view->viewID         = asset_manager->texture_catalog.texture_view_count;
        new_view->is_valid       = true;
        new_view->GPU_textureID  = 0; 
        new_view->uv_min         = &texture_data->uv_min;
        new_view->uv_max         = &texture_data->uv_max;

        asset_manager->texture_catalog.texture_view_count++;
    }

    return(new_view);
}

internal texture2D_t
s_asset_texture_create(asset_manager_t  *asset_manager,
                       zone_allocator_t *zone,
                       s32               width,
                       s32               height,
                       bitmap_format_t   format,
                       bool8             has_AA,
                       filter_type_t     filtering)
{
    texture2D_t result = {};
    result.bitmap      = s_asset_bitmap_create(zone, width, height, format);
    result.uv_min      = vec2_create_float(0.0, 0.0);
    result.uv_min      = vec2_create_float((float32)width, (float32)height);
    result.has_AA      = has_AA;
    result.filter_type = filtering;

    return(result);
}

internal texture2D_t
s_asset_texture_and_view_create(asset_manager_t  *asset_manager,
                                zone_allocator_t *zone,
                                s32               width,
                                s32               height,
                                bitmap_format_t   format,
                                bool8             has_AA,
                                filter_type_t     filtering)
{
    texture2D_t result = s_asset_texture_create(asset_manager, zone, width, height, format, has_AA, filtering);
    result.view        = s_asset_texture_view_generate(asset_manager, null, &result);

    return(result);
}

internal asset_handle_t
s_asset_texture_get(asset_manager_t *asset_manager, string_t asset_key)
{
    asset_handle_t result = {};

    asset_slot_t *valid_slot = (asset_slot_t *)c_hash_get_value(&asset_manager->texture_catalog.texture_hash, asset_key);
    result.type = AT_BITMAP;
    if(valid_slot)
    {
        result.is_valid               = true;
        result.type                   = AT_BITMAP;
        result.asset_slot             = valid_slot;

        texture2D_t *texture_data = &valid_slot->texture;
        if(texture_data->view)
        {
            // valid view
            result.texture = texture_data->view;
        }
        else
        {
            // generate a new view 
            texture_view_t *new_view = s_asset_texture_view_generate(asset_manager, valid_slot, texture_data);
            texture_data->view = new_view;
            result.texture     = new_view;
        }

        if(valid_slot->slot_state != ASS_LOADED && valid_slot->slot_state != ASS_QUEUED)
        {
            s_asset_texture_load_data(asset_manager, result);
        }
    }
    else
    {
        log_warning("Invalid texture key: '%s', could not find a texture with that name in our packaging system...\n", asset_key.data);
        result.is_valid = false;
        result.texture  = &asset_manager->texture_catalog.null_texture;
    }

    return(result);
}

internal void
s_asset_texture_load_data(asset_manager_t *asset_manager, asset_handle_t handle)
{
    Assert(handle.type == AT_BITMAP);

    asset_slot_t *slot_data = handle.asset_slot;
    Assert(slot_data->slot_state != ASS_LOADED);
    
    if(slot_data->asset_file_data_offset > 0)
    {
        slot_data->texture.bitmap.data = c_file_read_za(asset_manager->texture_catalog.texture_allocator,
                                                        asset_manager->asset_file_handle.filepath,
                                                        slot_data->asset_file_data_length,
                                                        slot_data->asset_file_data_offset,
                                                        ZA_TAG_TEXTURE);
    }
    else
    {
        // NOTE(Sleepster): Data wasn't in the asset file, load it from the filepath
        Assert(slot_data->filename.data != null);
        slot_data->texture.bitmap.data = c_file_read_za(asset_manager->texture_catalog.texture_allocator,
                                                        slot_data->filename,
                                                        READ_ENTIRE_FILE,
                                                        0,
                                                        ZA_TAG_TEXTURE);
    }
    u8 *data = stbi_load_from_memory(slot_data->texture.bitmap.data.data,
                                     slot_data->texture.bitmap.data.count,
                                     &slot_data->texture.bitmap.width,
                                     &slot_data->texture.bitmap.height,
                                     &slot_data->texture.bitmap.channels,
                                     BMF_RGBA32);
    s32 data_length = strlen((char *)data);
    slot_data->texture.bitmap.decompressed_data.data  = data;
    slot_data->texture.bitmap.decompressed_data.count = data_length;

    slot_data->slot_state            = ASS_LOADED;
    slot_data->texture.bitmap.format = BMF_RGBA32;
    slot_data->texture.bitmap.stride = 32;


    at_atlas_handler_add_texture(asset_manager, &asset_manager->texture_catalog.primary_handler, handle);
    //r_texture_make_gpu(&slot_data->texture, false, TAAFT_NEAREST);
}

internal inline void
s_asset_texture_destroy_data(asset_manager_t *asset_manager, asset_handle_t handle)
{
    bitmap_t *bitmap = &handle.asset_slot->texture.bitmap;

    c_za_DEBUG_validate_block_list(asset_manager->texture_catalog.texture_allocator);
    c_za_free(asset_manager->texture_catalog.texture_allocator, bitmap->data.data);

    free(handle.asset_slot->texture.bitmap.decompressed_data.data);
    handle.asset_slot->texture.bitmap.decompressed_data.count = 0;

    bitmap->data.count = 0;
    handle.asset_slot->slot_state = ASS_UNLOADED;
}

internal inline void
s_asset_texture_view_destroy(asset_manager_t *asset_manager, asset_handle_t handle)
{
    handle.texture->is_valid = false;
    memset(handle.texture, 0, sizeof(texture_view_t));
}
