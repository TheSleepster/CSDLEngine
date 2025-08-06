/* ========================================================================
   $File: ab_main.c $
   $Date: Sat, 02 Aug 25: 01:12PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb/stb_image.h>

#include <c_types.h>
#include <c_base.h>
#include <c_types.h>
#include <c_math.h>
#include <c_debug.h>
#include <c_memory.h>
#include <c_string.h>
#include <c_array.h>
#include <c_file_api.h>

#include "../os_platform_file.h"

#include <c_memory.c>
#include <c_string.c>
#include <c_array.c>
#include <c_file_api.c>

#include "../r_asset_texture.h"
#include "../r_asset_shader.h"

//STBIDEF stbi_uc *stbi_load_from_memory   (stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);

#define ASSET_FILE_MAGIC_VALUE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(a) << 24))
#define ASSET_FILE_VERSION 0

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

// NOTE(Sleepster): Little Endian byte ordering 
#pragma pack(push, 1)
typedef struct asset_file_header
{
    u32 magic_value; 
    u32 file_version;
    u32 file_size;
}asset_file_header_t;

typedef struct asset_file_TOC
{
    u32 asset_count;

    u32 bitmap_count;
    u32 shader_count;
    u32 sound_count;
    u32 font_count;

    string_t     *names;
    string_t     *filepaths;
    u32          *IDs;
    asset_type_t *type_table;
}asset_file_TOC_t;

typedef struct asset_file_slot
{
    u32                asset_ID;
    asset_type_t       asset_type;
    union
    {
        texture2D_t    texture;
        GPU_shader_t   shader;
        //loaded_sound_t sound;
        //dynamic_font_t render_font;
    };
}asset_file_slot_t;

typedef struct asset_file
{
    asset_file_header_t header;
    asset_file_TOC_t    table_of_contents;

    asset_file_slot_t  *asset_table;
}asset_file_t;
#pragma pack(pop)

typedef struct packer_state
{
    u32 current_ID;

    dynamic_array_t bitmap_file_data;
    dynamic_array_t shader_file_data;
    dynamic_array_t font_file_data;
    dynamic_array_t sound_file_data;
    
    asset_file_t    asset_file_data;
}packer_state_t;

global packer_state_t packer_state;

VISIT_FILES(get_file_from_dir)
{
    if(!visit_file_data->is_directory)
    {
        dynamic_array_t *array = &packer_state.bitmap_file_data;
        file_data_t file_data  = c_file_get_data(visit_file_data->fullname);
        string_t file_ext      = c_get_file_ext_from_path(file_data.filepath);

        if(c_string_compare(file_ext, STR(".ttf")))
        {
            packer_state.asset_file_data.table_of_contents.font_count++;
            array = &packer_state.font_file_data;
        }
        else if(c_string_compare(file_ext, STR(".png")))
        {
            packer_state.asset_file_data.table_of_contents.bitmap_count++;
            array = &packer_state.bitmap_file_data;
        }
        else if(c_string_compare(file_ext, STR(".wav")))
        {
            packer_state.asset_file_data.table_of_contents.sound_count++;
            array = &packer_state.sound_file_data;
        }
        else if(c_string_compare(file_ext, STR(".glsl")))
        {
            packer_state.asset_file_data.table_of_contents.shader_count++;
            array = &packer_state.shader_file_data;
        }

        packer_state.asset_file_data.table_of_contents.asset_count++;
        c_dynamic_array_append(array, file_data);
    }
}

internal void
asset_iterate_and_fill_TOC(dynamic_array_t *asset_array, asset_file_TOC_t *table, asset_type_t type)
{
    for(u32 index = 0;
        index < asset_array->indices_used;
        ++index)
    {
        file_data_t *file_data = c_dynamic_array_get(asset_array, index);
        table->names[packer_state.current_ID]      = file_data->filename;
        table->filepaths[packer_state.current_ID]  = file_data->filepath;
        table->IDs[packer_state.current_ID]        = packer_state.current_ID;
        table->type_table[packer_state.current_ID] = type;

        packer_state.current_ID++;
    }
}

internal void
asset_load_data_from_table(memory_arena_t    *arena,
                           asset_file_TOC_t  *table_of_contents,
                           asset_file_slot_t *asset_array,
                           u32                starting_index,
                           u32                ending_index)
{
    for(u32 asset_index = starting_index;
        asset_index < ending_index;
        ++asset_index)
    {
        string_t     filepath = table_of_contents->filepaths[asset_index];
        u32          ID       = table_of_contents->IDs[asset_index];
        asset_type_t type     = table_of_contents->type_table[asset_index];

        asset_file_slot_t *asset = asset_array + asset_index;
        asset->asset_ID          = ID;
        asset->asset_type        = type;

        switch(asset->asset_type)
        {
            case AT_BITMAP:
            {
                u32 *width         = &asset->texture.bitmap.width;
                u32 *height        = &asset->texture.bitmap.height;
                u32 *channels      = &asset->texture.bitmap.channels;
                u32 *channel_count = &asset->texture.bitmap.format;

                *channel_count     = 4;
                asset->texture.bitmap.data = c_file_read_arena(arena, filepath);  

                byte *data = stbi_load_from_memory(asset->texture.bitmap.data, );
            }break;
            default:
            {
                InvalidCodePath;
            }break;
        }
    }
}

int
main(int argc, char **argv)
{
    gc_setup();
    packer_state.bitmap_file_data = c_dynamic_array_create(file_data_t, 30);
    packer_state.shader_file_data = c_dynamic_array_create(file_data_t, 30);
    packer_state.font_file_data   = c_dynamic_array_create(file_data_t, 30);
    packer_state.sound_file_data  = c_dynamic_array_create(file_data_t, 30);

    // NOTE(Sleepster): In honor of DOOM(1993) 
    packer_state.asset_file_data.header.magic_value  = ASSET_FILE_MAGIC_VALUE('W', 'A', 'D', ' ');
    packer_state.asset_file_data.header.file_version = ASSET_FILE_VERSION;
    
    string_t resource_dir = STR("../run_tree/res");

    visit_file_data_t file_data = c_directory_create_visit_data(get_file_from_dir, true, null);
    c_directory_visit(resource_dir, &file_data);

    string_t     *names     = malloc(sizeof(string_t)     * packer_state.asset_file_data.table_of_contents.asset_count);
    string_t     *filepaths = malloc(sizeof(string_t)     * packer_state.asset_file_data.table_of_contents.asset_count);
    u32          *IDs       = malloc(sizeof(u32)          * packer_state.asset_file_data.table_of_contents.asset_count);
    asset_type_t *types     = malloc(sizeof(asset_type_t) * packer_state.asset_file_data.table_of_contents.asset_count);

    packer_state.asset_file_data.table_of_contents.names      = names;
    packer_state.asset_file_data.table_of_contents.filepaths  = filepaths;
    packer_state.asset_file_data.table_of_contents.IDs        = IDs;
    packer_state.asset_file_data.table_of_contents.type_table = types;

    asset_iterate_and_fill_TOC(&packer_state.bitmap_file_data, &packer_state.asset_file_data.table_of_contents, AT_BITMAP);
    asset_iterate_and_fill_TOC(&packer_state.shader_file_data, &packer_state.asset_file_data.table_of_contents, AT_SHADER);
    asset_iterate_and_fill_TOC(&packer_state.font_file_data,   &packer_state.asset_file_data.table_of_contents, AT_FONT);
    asset_iterate_and_fill_TOC(&packer_state.sound_file_data,  &packer_state.asset_file_data.table_of_contents, AT_SOUND);

    memory_arena_t bitmap_arena = c_arena_create(GB(1));
    memory_arena_t shader_arena = c_arena_create(GB(1));
    memory_arena_t font_arena   = c_arena_create(GB(1));
    memory_arena_t sound_arena  = c_arena_create(GB(1));

    u32 bitmap_starting_offset = 0;
    u32 bitmap_ending_index    = packer_state.asset_file_data.table_of_contents.bitmap_count;
    asset_load_data_from_table(&bitmap_arena,
                               &packer_state.asset_file_data.table_of_contents,
                                packer_state.asset_file_data.asset_table,
                                bitmap_starting_offset,
                                bitmap_ending_index);

    return(0);
}
