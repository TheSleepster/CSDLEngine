#if !defined(AB_PACKER_INFO_H)
/* ========================================================================
   $File: ab_packer_info.h $
   $Date: Sat, 09 Aug 25: 12:18PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define AB_PACKER_INFO_H

#include "../s_asset_manager.h"

#define ASSET_FILE_MAGIC_VALUE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
#define ASSET_FILE_VERSION 1UL

#define VERY_LARGE_NUMBER 4096

string_t valid_arguments[] =
{
    {.data = "--resource_dir", .count = 13},
    {.data = "--generate_enums", .count = 16},
    {.data = "--codegen_file_name", .count = 19},
    {.data = "--asset_file_name", .count = 17},
    {.data = "--file_ext", .count = 10},
    {.data = "--help", .count = 6},
};

global memory_arena_t packer_arena;

#pragma pack(push, 1)
typedef struct asset_file_header
{
    u32 magic_value;
    u32 version;
    u32 flags;
    u32 offset_to_table_of_contents;
}asset_file_header_t;

typedef struct asset_file_table_of_contents
{
    u32 magic_value;
    u32 reserved0;

    s64 entry_count;
    u64 reserved[6];
}asset_file_table_of_contents_t;

typedef struct asset_package_entry
{
    string_t     name;
    string_t     filepath;
    string_t     entry_data;
    u32          ID;
    u32          file_ID;
    asset_type_t type;

    u64          data_offset_from_start_of_file;
}asset_package_entry_t;
#pragma pack(pop)

typedef struct asset_packer
{
    file_t                 asset_file;
    
    string_builder_t       header;
    string_builder_t       data_entry;
    string_builder_t       table_of_contents;

    u32                    entry_count;

    u32                    next_entry_ID;
    u32                    next_entry_to_write;
    asset_package_entry_t  entries[VERY_LARGE_NUMBER];
}asset_packer_t;

#endif
