/* ========================================================================
   $File: c_file_api.c $
   $Date: Fri, 25 Jul 25: 01:25PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "c_file_api.h"

// NOTE(Sleepster): Errors from these calls are handled internally 
internal inline file_t
c_file_open(string_t filepath, bool8 create)
{
    return(os_file_open(filepath, create, false, false));
}

internal inline bool8 
c_file_close(file_t *file)
{
    bool8 result = os_file_close(file);
    if(!result)
    {
        log_error("Failed to close the file passed, filename: '%s'...", file->file_name.data);
    }

    file->handle = 0;
    return(result);
}

internal u32
c_file_get_read_size(file_t *file, u32 bytes_to_read, u32 file_offset)
{
    u32 result = 0;

    u32 file_size = os_file_get_size(file);
    if(bytes_to_read == MAX_U32)
    {
        // read the whole file
        result = file_size;
    }
    else if(bytes_to_read == 0)
    {
        // read what's left of the file from the offset
        result = file_size - file_offset;
    }
    else
    {
        // read the amount desired from the file
        result = bytes_to_read;
    }

    return(result);
}

// TODO(Sleepster): These should all call back to one routine and have the other memory methods call to this single call... this is stupid... 
internal string_t
c_file_read(string_t filepath, u32 bytes_to_read, u32 file_offset)
{
    string_t result = {};

    file_t file = os_file_open(filepath, false, false, false);
    if(file.handle != null)
    {
        bytes_to_read = c_file_get_read_size(&file, bytes_to_read, file_offset);
        u8 *data      = (u8 *)os_allocate_memory(sizeof(u8) * bytes_to_read);

        os_file_read(&file, data, file_offset, bytes_to_read);
        os_file_close(&file);

        result.data  = data;
        result.count = bytes_to_read;
    }
    return(result);
}

internal string_t
c_file_read_arena(memory_arena_t *arena, string_t filepath, u32 bytes_to_read, u32 file_offset)
{
    string_t result = {};
    
    file_t file = os_file_open(filepath, false, false, false);
    if(file.handle != null)
    {
        bytes_to_read = c_file_get_read_size(&file, bytes_to_read, file_offset);
        u8 *data      = c_arena_push_size(arena, sizeof(u8) * bytes_to_read);

        os_file_read(&file, data, file_offset, bytes_to_read);
        os_file_close(&file);

        result.data  = data;
        result.count = bytes_to_read;
    }
    return(result);
}

internal string_t
c_file_read_za(zone_allocator_t *zone, string_t filepath, u32 bytes_to_read, u32 file_offset, za_allocation_tag_t tag)
{
    string_t result = {};
    
    file_t file = os_file_open(filepath, false, false, false);
    if(file.handle != null)
    {
        bytes_to_read = c_file_get_read_size(&file, bytes_to_read, file_offset);
        u8 *data      = c_za_alloc(zone, sizeof(u8) * bytes_to_read, tag);

        os_file_read(&file, data, file_offset, bytes_to_read);
        os_file_close(&file);

        result.data  = data;
        result.count = bytes_to_read;
    }
    return(result);
}

internal bool8 
c_file_open_and_write(string_t filepath, void *data, s64 bytes_to_write, bool8 overwrite)
{
    bool8 result = false;
    
    file_t file = os_file_open(filepath, true, overwrite, false);
    if(file.handle != null)
    {
        os_file_write(&file, data, bytes_to_write);
        result = true;
    }

    return(result);
}

internal inline bool8 
c_file_write(file_t *file, void *data, s64 bytes_to_write)
{
    return(os_file_write(file, data, bytes_to_write));
}

internal inline bool8 
c_file_write_string(file_t *file, string_t data)
{
    return(c_file_write(file, data.data, data.count));
}

internal s64
c_file_get_size(string_t filepath)
{
    s64 result = 0;
    
    file_t file = os_file_open(filepath, false, false, false);
    if(file.handle != null)
    {
        result = os_file_get_size(&file);
        os_file_close(&file);
    }

    return(result);
}

internal inline file_data_t
c_file_get_data(string_t filepath)
{
    return(os_file_get_modtime_and_size(filepath));
}

internal inline bool8
c_file_replace_or_rename(string_t old_file, string_t new_file)
{
    bool8 result = false;
    result = os_file_replace_or_rename(old_file, new_file);

    return(result);
}

internal inline mapped_file_t
c_file_map(string_t filepath)
{
    return(os_file_map(filepath));
}

internal inline bool8
c_file_unmap(mapped_file_t *map_data)
{
    return(os_file_unmap(map_data));
}

/////////////////
// DIRECTORY
////////////////
internal visit_file_data_t
c_directory_create_visit_data(visit_files_pfn_t *function, bool8 recursive, void *user_data)
{
    visit_file_data_t result = {};

    result.function  = function;
    result.recursive = recursive;
    result.user_data = user_data;

    return(result);
}

internal inline void
c_directory_visit(string_t filepath, visit_file_data_t *visit_file_data)
{
    return(os_directory_visit(filepath, visit_file_data));
}

internal inline bool8
c_directory_exists(string_t filepath)
{
    return(os_directory_exists(filepath));
}
