/* ========================================================================
   $File: ab_main.c $
   $Date: Sat, 02 Aug 25: 01:12PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <c_types.h>
#undef internal
#define internal static

#include <c_base.h>
#include <c_types.h>
#include <c_math.h>
#include <c_log_assert.h>
#include <c_memory_arena.h>
#include <c_string.h>
#include <c_array.h>

#include <os_platform_file.h>

#include <s_multithreading_work_queue.h>
#include <c_multithreading_primitives.h>

#include <c_zone_allocator.h>
#include <c_hash_table.h>
#include <c_file_api.h>

#include <c_zone_allocator.cpp>
#include <c_memory_arena.cpp>
#include <c_string.cpp>
#include <c_array.cpp>
#include <c_file_api.cpp>
#include <c_hash_table.cpp>

#include "../r_asset_texture.h"
#include "../r_asset_shader.h"

#include "ab_packer_info.h"
#include "../s_asset_manager.h"

internal void
afb_add_entry(asset_packer_t *packer,
              string_t        name,
              string_t        filepath,
              string_t        data,
              asset_type_t    type)
{
    // NOTE(Sleepster): zeroth entry is null 
    asset_package_entry_t *entry = &packer->entries[packer->next_entry_to_write];
    u64 ID = c_fnv_hash_value(name.data, name.count, default_fnv_hash_value);

    entry->name       = c_string_make_copy(&packer_arena, name);
    entry->filepath   = c_string_make_copy(&packer_arena, filepath);
    entry->entry_data = c_string_make_copy(&packer_arena, data);
    entry->ID         = (ID % MANAGER_HASH_TABLE_SIZE + MANAGER_HASH_TABLE_SIZE) % MANAGER_HASH_TABLE_SIZE;
    entry->file_ID    = packer->next_entry_to_write;
    entry->type       = type;

    packer->next_entry_to_write++;
}

internal void
afb_file_write(asset_packer_t *packer)
{
    StaticAssert(sizeof(asset_file_header_t) == 16);

    u32 offset = sizeof(asset_file_header_t);
    for(u32 packer_entry_index = 0;
        packer_entry_index < packer->next_entry_to_write;
        ++packer_entry_index)
    {
        asset_package_entry_t *entry = &packer->entries[packer_entry_index];

        entry->data_offset_from_start_of_file = offset;
        offset += entry->entry_data.count;
    }

    const u32 FILE_MAGIC_VALUE = ASSET_FILE_MAGIC_VALUE('W', 'A', 'D', ' ');
    const u32 FILE_VERSION     = ASSET_FILE_VERSION;
    const u32 FLAGS_DWORD      = 100;
    const u32 OFFSET_TO_TOC    = offset;

    c_string_builder_append_value(&packer->header, (void*)&FILE_MAGIC_VALUE, sizeof(u32));
    c_string_builder_append_value(&packer->header, (void*)&FILE_VERSION,     sizeof(u32));
    c_string_builder_append_value(&packer->header, (void*)&FLAGS_DWORD,      sizeof(u32));
    c_string_builder_append_value(&packer->header, (void*)&OFFSET_TO_TOC,    sizeof(u32));

    c_string_builder_write_to_file(&packer->asset_file, &packer->header);

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
    }

    asset_file_table_of_contents_t table_of_contents = {};
    table_of_contents.magic_value = ASSET_FILE_MAGIC_VALUE('t', 'o', 'c', 'd');
    table_of_contents.entry_count = packer->next_entry_to_write;

    c_string_builder_append_value(&packer->table_of_contents, &table_of_contents, sizeof(asset_file_table_of_contents_t));
/* THE ENTRIES ARE CONSTRUCTED HERE:
 *
 * Entries should be read like so...
 * - 4-byte value to indiciate the filename's length...
 * - The file's name (zero terminated...)
 * - 4-byte integer to indicate the filepath's length...
 * - The file's path (again, zero terminated...)
 * - 4-byte integer to indicate the entry's data length...
 * - 4-byte integer to indicate the entry's ID....
 * - 4-byte integer to indicate the entry's ID in the file...
 * - 4-byte integer to indicate the entry's asset type...
 * - 8-byte integer to indicate the entry's data offset...
 */    
    s8 null_term = 0;
    for(u32 packer_entry_index = 0;
        packer_entry_index < packer->next_entry_to_write;
        ++packer_entry_index)
    {
        asset_package_entry_t *entry = &packer->entries[packer_entry_index];

        c_string_builder_append_value(&packer->table_of_contents, &entry->name.count,                     sizeof(u32));
        c_string_builder_append_value(&packer->table_of_contents,  entry->name.data,                      entry->name.count);
        c_string_builder_append_value(&packer->table_of_contents, &null_term,                             sizeof(s8));

        c_string_builder_append_value(&packer->table_of_contents, &entry->filepath.count,                 sizeof(u32));
        c_string_builder_append_value(&packer->table_of_contents,  entry->filepath.data,                  entry->filepath.count);
        c_string_builder_append_value(&packer->table_of_contents, &null_term,                             sizeof(s8));

        c_string_builder_append_value(&packer->table_of_contents, &entry->entry_data.count,               sizeof(u32));
        c_string_builder_append_value(&packer->table_of_contents, &entry->ID,                             sizeof(u32));
        c_string_builder_append_value(&packer->table_of_contents, &entry->file_ID,                        sizeof(u32));
        c_string_builder_append_value(&packer->table_of_contents, &entry->type,                           sizeof(asset_type_t));
        c_string_builder_append_value(&packer->table_of_contents, &entry->data_offset_from_start_of_file, sizeof(u64));
    }

    c_string_builder_write_to_file(&packer->asset_file, &packer->table_of_contents);
    packer->entry_count = packer->next_entry_to_write;
}

internal void
generate_asset_file_enums(asset_packer_t *packer, string_t filename)
{
    log_info("Writing asset enum to file: '%s'...", filename.data);
    
    file_t codegen_file = c_file_open(filename, true);

    string_builder_t enum_builder = {};
    c_string_builder_init(&enum_builder, MB(100));

    string_t starting_file = STR("// This file is generated by the asset_builder...\n\n");
    c_string_builder_append(&enum_builder, starting_file);

    string_t enum_prefix = STR("\tAFI_");
    string_t enum_name   = STR("typedef enum asset_file_indices\n{\n");
    c_string_builder_append(&enum_builder, enum_name);

    for(u32 enum_index   = 0;
        enum_index < packer->next_entry_to_write;
        ++enum_index)
    {
        asset_package_entry_t *entry = &packer->entries[enum_index];
        string_t enum_member = c_string_concat(&packer_arena, enum_prefix, entry->name);
                 enum_member = c_string_concat(&packer_arena, enum_member, STR(",\n"));
        c_string_builder_append(&enum_builder, enum_member);
    }
    c_string_builder_append(&enum_builder, STR("\tAFI_Count\n"));
    c_string_builder_append(&enum_builder, STR("}asset_file_indices_t;\n"));

    c_string_builder_write_to_file(&codegen_file, &enum_builder);
    c_file_close(&codegen_file);
}

VISIT_FILES(get_resource_dir_files)
{
    string_t filename = visit_file_data->filename;
    string_t filepath = visit_file_data->fullname;
    string_t file_ext = c_string_get_file_ext_from_path(filename);

    string_t filename_no_ext = filename;
    filename_no_ext.count -= file_ext.count;

    asset_type_t type = AT_NONE;
    if(c_string_compare(file_ext, STR(".ttf")))
    {
        type = AT_FONT;
    }
    else if(c_string_compare(file_ext, STR(".wav")))
    {
        type = AT_SOUND;
    }
    else if(c_string_compare(file_ext, STR(".png")))
    {
        type = AT_BITMAP;
    }
    else if(c_string_compare(file_ext, STR(".glsl")))
    {
        type = AT_SHADER;
    }
    
    asset_packer_t *packer = (asset_packer_t*)user_data; 
    if(type != AT_NONE)
    {
        string_t data = c_file_read(filepath, READ_ENTIRE_FILE, 0);
        afb_add_entry(packer, filename_no_ext, filepath, data, type);
    }
}


int
main(int argc, char **argv)
{
    gc_setup();
    packer_arena = c_arena_create(GB(2));

    string_t packed_file_name  = STR("asset_file");
    string_t resource_dir      = STR("../run_tree/res");
    string_t output_dir        = STR("../run_tree/res/");
    string_t codegen_file_name = STR("../code/GENERATED_asset_enums.h");
    string_t file_ext          = STR(".wad");
    bool8    generate_enums    = false;
    
   // NOTE(Sleepster): The fact that Windows doesn't have anything like getopt_long is painful...
    for(s32 arg_index = 1;
        arg_index < argc;
        ++arg_index)
    {
        char *next_argument = argv[arg_index];
        string_t arg_string = c_string_create(next_argument);
        string_t command_name;

        s32 equals_index = c_string_find_first_char_from_left(arg_string, '=');
        if(equals_index != -1)
        {
            command_name = c_string_sub_from_left(arg_string, equals_index);
        }

        string_t prefix = arg_string;
        prefix.count    = 2;
        if(!c_string_compare(prefix, STR("--")))
        {
            log_error("Passed Command: '%s', is not valid... please append with '--' or use '--help' for more information...", next_argument);
            continue;
        }

        command_name = c_string_make_copy(&packer_arena, arg_string);
        command_name.count = equals_index - 1;
        for(u32 v_arg_index = 0;
            v_arg_index < ArrayCount(valid_arguments);
            ++v_arg_index)
        {
            string_t valid_argument = valid_arguments[v_arg_index];
            if(c_string_compare(valid_argument, command_name))
            {
                char first_char = command_name.data[2];
                switch(first_char)
                {
                    case 'r':
                    {
                        if(equals_index == -1) log_error("Attempted to call command '--resource_dir'...\nhowever the passed argument does not contain an '='...");
                        string_t directory_name = c_string_sub_from_left(arg_string, equals_index + 1);
                        log_info("Set resource_dir to: '%s'...\n", directory_name.data);

                        resource_dir = directory_name;
                        goto break_out;
                    }break;
                    case 'g':
                    {
                        generate_enums = true;
                        log_info("Generating asset enum data...\n");
                        goto break_out;
                    }break;
                    case 'c':
                    {
                        if(equals_index == -1) log_error("Attempted to call command '--codegen_file_name'...\nhowever the passed argument does not contain an '='...");
                        string_t new_filename = c_string_sub_from_left(arg_string, equals_index);

                        codegen_file_name = new_filename;
                        goto break_out;
                    }break;
                    case 'a':
                    {
                        if(equals_index == -1) log_error("Attempted to call command '--asset_file_name'...\nhowever the passed argument does not contain an '='...");
                        string_t new_filename = c_string_sub_from_left(arg_string, equals_index + 1);

                        packed_file_name = new_filename;
                        goto break_out;
                    }break;
                    case 'f':
                    {
                        if(equals_index == -1) log_error("Attempted to call command '--file_ext'...\nhowever the passed argument does not contain an '='...");
                        string_t new_ext = c_string_sub_from_left(arg_string, equals_index + 1);

                         file_ext = new_ext;
                        goto break_out;
                    }break;
                    case 's':
                    {
                        if(equals_index == -1) log_error("Attempted to call command '--set_output_dir'...\nhowever the passed argument does not contain an '='...");
                        string_t new_output_dir = c_string_sub_from_left(arg_string, equals_index + 1);
                        output_dir = new_output_dir;
                        log_info("Set output_dir to: '%s'...\n", new_output_dir.data);

                        goto break_out;
                    }
                    case 'h':
                    {
                        printf("-------Helpful Information-------\n\n");
                        printf("These are commands used by the program:\n\n");
                        for(u32 print_valid_arg = 0;
                            print_valid_arg < ArrayCount(valid_arguments);
                            ++print_valid_arg)
                        {
                            string_t va_argument = valid_arguments[print_valid_arg];
                            printf("'%s'\n", va_argument.data);
                        }

                        printf("\nThe arguments do the following:\n");
                        printf("Argument: '--resource_dir'\nSets the directory the packer will read assets from... By default this is: '../run_tree/res'\n\n");
                        printf("Argument: '--generate_enums'\nCreates an enum for the indices of the asset_entry_table in the package table of contents... By default this behavior is: false.\n\n");
                        printf("Argument: '--codegen_file_name'\nSets the name of the generated file... By default it is: '../code/GENERATED_asset_enums.h'.\n\n");
                        printf("Argument: '--file_ext'\nSets the asset file's file extension... By default it is: '.wad'\n\n");
                        printf("Argument: '--help'\nYou already know...\n\n");
                        printf("---------------------------------\n");

                        return(0);
                    }break;
                }
            }
        }
       // NOTE(Sleepster): yes this is gross... 
break_out:
        continue;
    }

    output_dir = c_string_concat(&packer_arena, output_dir, STR("/"));
    string_t full_packed_filename = c_string_concat(&packer_arena, output_dir, packed_file_name);
    full_packed_filename = c_string_concat(&packer_arena, full_packed_filename, file_ext);
    log_info("Packing assets to file: '%s'...", full_packed_filename.data);
    
    asset_packer_t packer = {};
    packer.next_entry_to_write = 0;
    c_string_builder_init(&packer.header, MB(100));
    c_string_builder_init(&packer.table_of_contents, MB(100));

    packer.asset_file = c_file_open(full_packed_filename, true);

    visit_file_data_t file_data = c_directory_create_visit_data(get_resource_dir_files, true, &packer);
    c_directory_visit(resource_dir, &file_data);

    afb_file_write(&packer);
    c_file_close(&packer.asset_file);

    if(generate_enums)
    {
        generate_asset_file_enums(&packer, codegen_file_name);
    }
}
