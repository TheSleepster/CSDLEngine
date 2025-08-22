/* ========================================================================
   $File: r_asset_dynamic_render_font.c $
   $Date: Mon, 18 Aug 25: 02:27PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "r_asset_dynamic_render_font.h"

internal asset_handle_t 
s_asset_font_get(asset_manager_t *asset_manager, string_t font_name)
{
    asset_handle_t result = {};
    asset_slot_t *valid_slot = c_hash_get_value(&asset_manager->font_catalog.font_hash, font_name);
    if(valid_slot)
    {
        result.is_valid       =  true;
        result.type           =  AT_FONT;
        result.asset_slot     =  valid_slot;
        result.font           = &valid_slot->render_font;
        result.font->filename =  valid_slot->filename;
    }
    else
    {
        log_error("Font with the name: '%s' is not found in the asset data hash...\n", font_name.data);
    }

    return(result);
}

internal u32
s_UTF8_convert_UTF32(u8 *character)
{
    u32 result = 0;
    u8  continuation_bytes = UTF8_trailing_bytes[character[0]];
    if(continuation_bytes + 1 < 1000)
    {
        u32 utf32_char = character[0] & UTF8_initial_bytemask[continuation_bytes];
        for(s32 byte = 1;
            byte < continuation_bytes;
            ++byte)
        {
            utf32_char  = utf32_char << 6;
            utf32_char |= character[byte] & 0x3F;
        }

        if(utf32_char > UTF32_MAX_CHARACTER) utf32_char = UTF32_REPLACEMENT_CHARACTER;

        result = utf32_char;
    }
    return(result);
}

internal vec2_t
s_font_atlas_find_next_free_line(dynamic_render_font_page_t *page, s32 glyph_width, s32 row_height)
{
    vec2_t result;
    s32 desired_x = page->bitmap_cursor_x + glyph_width;
    if(desired_x > page->font_atlas.bitmap.width)
    {
        page->bitmap_cursor_x  = 0;
        page->bitmap_cursor_y += page->owner_varient->max_ascender;

        result = vec2_create_float(0, page->bitmap_cursor_y);
    }
    else
    {
        result = vec2_create_float(page->bitmap_cursor_x + 1, page->bitmap_cursor_y + 1);
    }

    return(result);
}

internal void 
s_font_copy_glyph_data_to_page_bitmap(asset_manager_t            *asset_manager,
                                      memory_arena_t             *arena,
                                      dynamic_render_font_page_t *page,
                                      font_glyph_t               *glyph)
{
    if(!page->bitmap_valid)
    {
        page->font_atlas.bitmap.format     = BMF_RGBA32;
        page->font_atlas.bitmap.channels   = 4;
        page->font_atlas.bitmap.width      = 4096;
        page->font_atlas.bitmap.height     = 4096;
        page->font_atlas.bitmap.stride     = 32;
        page->font_atlas.bitmap.data.data  = c_arena_push_size(arena, (4096 * 4096 * 4) * sizeof(u8));
        page->font_atlas.bitmap.data.count = (4096 * 4096 * 4) * sizeof(u8);
        page->font_atlas.uv_min            = vec2_create_float(0.0, 0.0);
        page->font_atlas.uv_max            = vec2_create_float(1.0, 1.0);
        page->font_atlas.has_AA            = false;
        page->font_atlas.filter_type       = TAAFT_NEAREST;
        page->font_atlas.view              = s_asset_texture_view_generate(asset_manager, null, &page->font_atlas);
        
        page->bitmap_valid = true;
    }

    FT_Face font_face = page->owner_varient->parent->font_face;

    glyph->offset_x   = (s16)(font_face->glyph->bitmap_left);
    glyph->offset_y   = (s16)(font_face->glyph->bitmap_top);

    glyph->advance    = (s16)(font_face->glyph->advance.x >> 6);
    glyph->ascent     = (s16)(font_face->glyph->metrics.horiBearingY >> 6);

    s32 glyph_width   = font_face->glyph->bitmap.width;
    s32 row_height    = font_face->glyph->bitmap.rows;

    glyph->glyph_render_size = vec2_create_float(glyph_width, row_height);
    vec2_t bitmap_offset     = s_font_atlas_find_next_free_line(page, glyph_width, row_height);
    glyph->atlas_offset      = vec2_create_float((float32)bitmap_offset.x / (float32)page->font_atlas.bitmap.width,
                                                 (float32)bitmap_offset.y / (float32)page->font_atlas.bitmap.height);

    glyph->glyph_size = vec2_create_float((float32)glyph_width / (float32)page->font_atlas.bitmap.width,
                                          (float32)row_height  / (float32)page->font_atlas.bitmap.height);
    
    for(s32 row = 0;
        row < row_height;
        ++row)
    {
        for(s32 column = 0;
            column < glyph_width;
            ++column)
        {
            uint8  source = font_face->glyph->bitmap.buffer[(row_height- 1 - row) * font_face->glyph->bitmap.pitch + column];
            uint8 *dest   = (u8 *)page->font_atlas.bitmap.data.data + (((u32)bitmap_offset.y + row) * page->font_atlas.bitmap.width + ((uint32)bitmap_offset.x + column)) * 4;

            dest[0] = source;
            dest[1] = source;
            dest[2] = source;
            dest[3] = source;
        }
    }
    
    page->bitmap_cursor_x += font_face->glyph->bitmap.width;
}

internal font_glyph_t*
s_asset_font_get_utf8_glyph(asset_manager_t *asset_manager, dynamic_render_font_varient_t *varient, u8 *character)
{
    font_glyph_t *result = null;

    dynamic_render_font_page_t *valid_page = 0;
    u32 UTF32_char = s_UTF8_convert_UTF32(character);
    if(UTF32_char)
    {
        string_t temp = STR((char *)&UTF32_char);
        for(dynamic_render_font_page_t *page = varient->first_page;
            page;
            page = page->next_page)
        {
            result = (font_glyph_t*)c_hash_get_value(&page->glyph_lookup, temp);
            if(result)
            {
                valid_page = page;
                break;
            }
        }

        if(!result)
        {
            dynamic_render_font_page_t *last_page = 0;
            for(dynamic_render_font_page_t *page = varient->first_page;
                page;
                page = page->next_page)
            {
                if(!page->is_full)
                {
                    valid_page = page;
                }

                if(page->next_page == null) last_page = page;
            }
            
            if(!valid_page)
            {
                dynamic_render_font_page_t *new_page = null;
                last_page->next_page = c_arena_push_struct(&varient->parent->font_arena, dynamic_render_font_page_t);

                last_page->next_page->glyph_lookup   = c_hash_table_create_ma(&varient->parent->font_arena, 1000, sizeof(font_glyph_t));
                last_page->next_page->next_page      = null;
                last_page->next_page->owner_varient  = varient;

                last_page->next_page = new_page;
            }

            FT_Error error = FT_Set_Pixel_Sizes(varient->parent->font_face, 0, varient->pixel_size);
            Assert(!error);

            u32 glyph_index = 0;
            if(UTF32_char)
            {
                glyph_index = FT_Get_Char_Index(varient->parent->font_face, UTF32_char);
                if(!glyph_index)
                {
                    log_warning("UTF32 character '%d' cannot be found...\n", UTF32_char);
                    glyph_index = varient->default_unknown_character;
                }

                error = FT_Load_Glyph(varient->parent->font_face, glyph_index, FT_LOAD_RENDER);
                Assert(!error);
            }
            else
            {
                Assert(glyph_index >= 0);
                error = FT_Load_Glyph(varient->parent->font_face, glyph_index, FT_LOAD_RENDER);
                Assert(!error);
            }

            font_glyph_t *glyph = c_arena_push_struct(&varient->parent->font_arena, font_glyph_t);

            glyph->hash_key   = c_string_make_copy(&varient->parent->font_arena, temp);
            glyph->owner_page = valid_page;

            s_font_copy_glyph_data_to_page_bitmap(asset_manager, &varient->parent->font_arena, valid_page, glyph);
            c_hash_insert_kv_pair(&valid_page->glyph_lookup, glyph->hash_key, glyph);
            result = glyph;

            valid_page->bitmap_dirty = true;
        }
    }
    
    return(result);
}

internal string_t
s_asset_font_load_data(memory_arena_t *arena, asset_manager_t *asset_manager, asset_handle_t handle)
{
    string_t result;

    asset_slot_t *slot = handle.asset_slot;
    if(slot->asset_file_data_offset > 0)
    {
        result = c_file_read_arena(arena,
                                   asset_manager->asset_file_handle.filepath,
                                   slot->asset_file_data_length,
                                   slot->asset_file_data_offset);
    }
    else
    {
        result = c_file_read_arena(arena,
                                   slot->filename,
                                   READ_ENTIRE_FILE,
                                   slot->asset_file_data_offset);
    }

    return(result);
}

internal inline bool8
s_asset_font_set_unknown_character(dynamic_render_font_varient_t *varient, u32 UTF32_index)
{
    bool8 result = false;
    
    u32 glyph_index = FT_Get_Char_Index(varient->parent->font_face, UTF32_index);
    if(glyph_index)
    {
        varient->default_unknown_character = glyph_index;
        result = true;
    }

    return(result);
}

internal dynamic_render_font_varient_t*
s_asset_font_create_at_size(asset_manager_t *asset_manager, asset_handle_t handle, u32 size)
{
    dynamic_render_font_varient_t *result = null;
    
    dynamic_render_font_t *font = handle.font;
    if(!font->is_initialized)
    {
        font->font_arena  = c_arena_create(MB(200));
        font->loaded_data = s_asset_font_load_data(&font->font_arena, asset_manager, handle);
        font->pixel_sizes = c_dynamic_array_create(dynamic_render_font_varient_t, 20);

        Assert(font->loaded_data.data);
        font->is_initialized = true;
    }
    FT_Error error = FT_New_Memory_Face(asset_manager->font_catalog.font_lib,
                                        font->loaded_data.data,
                                        font->loaded_data.count,
                                        0,
                                       &font->font_face);
    if(error == 0)
    {
        result = c_arena_push_struct(&font->font_arena, dynamic_render_font_varient_t);

        result->parent     = font;
        result->first_page = c_arena_push_struct(&font->font_arena, dynamic_render_font_page_t);
        
        result->first_page->glyph_lookup  = c_hash_table_create_ma(&result->parent->font_arena, 1000, sizeof(font_glyph_t));
        result->first_page->owner_varient = result;
        result->first_page->next_page     = null;

        error = FT_Set_Pixel_Sizes(font->font_face, 0, size);
        Assert(!error);

        float64 font_scale_to_pixels = font->font_face->size->metrics.y_scale / (64.0 * 65536.0);
        result->pixel_size    = size;
        result->line_spacing  = (s64)floor(font_scale_to_pixels * font->font_face->height    + 0.5);
        result->max_ascender  = (s64)floor(font_scale_to_pixels * font->font_face->bbox.yMax + 0.5);
        result->max_descender = (s64)floor(font_scale_to_pixels * font->font_face->bbox.yMin + 0.5);

        // NOTE(Sleepster): Using 'm' as the baseline character
        u32 glyph_index = FT_Get_Char_Index(font->font_face, 'm');
        if(glyph_index)
        {
            FT_Load_Glyph(font->font_face, glyph_index, FT_LOAD_DEFAULT);
            result->y_center_offset = (s32)(0.5f * FT_ROUND(font->font_face->glyph->metrics.horiBearingY) + 0.5f);
        }

        glyph_index = FT_Get_Char_Index(font->font_face, 'M');
        if(glyph_index)
        {
            FT_Load_Glyph(font->font_face, glyph_index, FT_LOAD_DEFAULT);
            result->em_width = FT_ROUND(font->font_face->glyph->metrics.width);
        }

        glyph_index = FT_Get_Char_Index(font->font_face, 'T');
        if(glyph_index)
        {
            FT_Load_Glyph(font->font_face, glyph_index, FT_LOAD_DEFAULT);
            result->typical_ascender = FT_ROUND(font->font_face->glyph->metrics.horiBearingY);
        }

        glyph_index = FT_Get_Char_Index(font->font_face, 'g');
        if(glyph_index)
        {
            FT_Load_Glyph(font->font_face, glyph_index, FT_LOAD_DEFAULT);
            result->typical_descender = FT_ROUND(font->font_face->glyph->metrics.horiBearingY - font->font_face->glyph->metrics.height);
        }

        error = FT_Select_Charmap(font->font_face, FT_ENCODING_UNICODE);
        if(error)
        {
            log_error("Failure to set the charmap to unicode.... supplied font does not support Unicode...\n");
        }

        bool8 success = s_asset_font_set_unknown_character(result,        0xfffd); // Replacement character
        if(!success) success = s_asset_font_set_unknown_character(result, 0x2022); // bullet char
        if(!success) success = s_asset_font_set_unknown_character(result, (u32)'?');
        if(!success) log_warning("Unable to set the unknown character for this font...\n");

        c_dynamic_array_append_value(&font->pixel_sizes, *result);
    }
    else
    {
        log_error("Freetype failed too create a new memory face...\n");
    }
    
    return(result);
}

internal dynamic_render_font_varient_t*
s_asset_font_get_at_size(asset_manager_t *asset_manager, asset_handle_t handle, u32 size)
{
    Assert(handle.type == AT_FONT);
    dynamic_render_font_varient_t *result = null;
    
    dynamic_render_font_t *font = handle.font;
    for(u32 pixel_size = 0;
        pixel_size < font->pixel_sizes.indices_used;
        ++pixel_size)
    {
        dynamic_render_font_varient_t *varient = c_dynamic_array_get(&font->pixel_sizes, pixel_size);
        if(size == varient->pixel_size)
        {
            result = varient;
            break;
        }
    }

    if(!result)
    {
        result = s_asset_font_create_at_size(asset_manager, handle, size);
    }

    return(result);
}
