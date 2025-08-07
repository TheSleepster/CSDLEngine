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

#define ASSET_FILE_MAGIC_VALUE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
#define ASSET_FILE_VERSION 1UL

#define VERY_LARGE_NUMBER 4096

#pragma pack(push, 1)
typedef struct asset_file_header
{
    u32 magic_value;
    u32 version;
    u32 flags;
    u32 byte_offset_to_table_of_contents;
}asset_file_header_t;

typedef struct asset_package_entry
{
    string_t name;
    string_t entry_data;
    u32      ID;

    u64      offset_from_start_of_file;
}asset_package_entry_t;
#pragma pack(pop)

typedef struct asset_packer
{
    file_t                 asset_file;
    
    string_builder_t       header;
    string_builder_t       table_of_contents;

    u32                    next_entry_ID;
    u32                    next_entry_to_write;
    asset_package_entry_t  entries[VERY_LARGE_NUMBER];
}asset_packer_t;

internal void
afb_add_entry(asset_packer_t *packer, string_t name, string_t data)
{
    asset_package_entry_t *entry = &packer->entries[packer->next_entry_to_write++];
    entry->name       = c_string_copy(&global_context.temporary_arena, name);
    entry->entry_data = c_string_copy(&global_context.temporary_arena, data);
    entry->ID         = packer->next_entry_ID++;
}

internal void
afb_file_write(asset_packer_t *packer)
{
    StaticAssert(sizeof(asset_file_header_t) == 16);

    const u32 FILE_MAGIC_VALUE = ASSET_FILE_MAGIC_VALUE('W', 'A', 'D', ' ');
    const u32 FILE_VERSION     = ASSET_FILE_VERSION;
    const u32 FLAGS_DWORD      = 100;

    c_string_builder_append_value(&packer->header, &FILE_MAGIC_VALUE, sizeof(u32));
    c_string_builder_append_value(&packer->header, &FILE_VERSION,     sizeof(u32));
    c_string_builder_append_value(&packer->header, &FLAGS_DWORD,      sizeof(u32));

    c_string_builder_write_to_file(&packer->asset_file, &packer->header);
    u32 offset_from_start_of_file = c_string_builder_get_string_length(&packer->header);
    Assert(offset_from_start_of_file == sizeof(asset_file_header_t));

    for(u32 packer_entry_index = 0;
        packer_entry_index < packer->next_entry_to_write;
        ++packer_entry_index)
    {
        asset_package_entry_t *entry = &packer->entries[packer_entry_index];
        bool8 success = c_file_write_string(&packer->asset_file, entry->entry_data);
        if(!success)
        {
            log_error("Failed to write entry: '%d'(%s) to the file...\n", packer_entry_index, entry->name);
        }

        entry->offset_from_start_of_file = offset_from_start_of_file;
        offset_from_start_of_file       += entry->entry_data.count;
    }
}

int
main(int argc, char **argv)
{
    gc_setup();
    asset_packer_t packer;
    c_string_builder_init(&packer.header, MB(100));
    c_string_builder_init(&packer.header, MB(100));

    packer.asset_file = c_file_open(STR("asset_file.wad"), true);
    string_t resource_dir = STR("../run_tree/res");

    afb_file_write(&packer);
    c_file_close(&packer.asset_file);
}
