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

#include <c_memory.c>
#include <c_string.c>
#include <c_array.c>

#include "../os_platform_file.h"

#include "../r_asset_texture.h"
#include "../r_asset_shader.h"

#define ASSET_FILE_MAGIC_VALUE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(a) << 24))
#define ASSET_FILE_VERSION 0

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

typedef struct asset_file_slot
{
    asset_type_t       asset_type;
    union
    {
        texture2D_t    texture;
        GPU_shader_t   shader;
        //loaded_sound_t sound;
        //dynamic_font_t render_font;
    };
}asset_file_slot_t;

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
    u32 font_count;
    u32 sound_count;

    string_t *names;
    u32      *IDs;
}asset_file_TOC_t;

typedef struct asset_file
{
    asset_file_header_t header;
    asset_file_TOC_t    table_of_contents;

    asset_file_slot_t  *asset_table;
}asset_file_t;
#pragma pack(pop)

int
main(int argc, char **argv)
{
    asset_file_t asset_file_data = {};
    asset_file_data.header.magic_value  = ASSET_FILE_MAGIC_VALUE('A', 'D', 'I', 'H');
    asset_file_data.header.file_version = ASSET_FILE_VERSION;
    
    string_t resource_dir = STR("res/");

    printf("testiong...\n");

    return(0);
}
