/* ========================================================================
   $File: c_file_api.c $
   $Date: Fri, 25 Jul 25: 01:25PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

// NOTE(Sleepster): Errors from these calls are handled internally 
internal string_t
c_file_read(string_t filepath)
{
    string_t result = {};
    
    file_t file = os_file_open(filepath, false, false, false);
    if(file.handle != null)
    {
        s64 file_size = os_file_get_size(&file);
        u8 *data = malloc(sizeof(u8) * file_size);

        os_file_read(&file, data, file_size);
        os_file_close(&file);

        result.data  = data;
        result.count = file_size;
    }
    return(result);
}

internal string_t
c_file_read_arena(memory_arena_t *arena, string_t filepath)
{
    string_t result = {};
    
    file_t file = os_file_open(filepath, false, false, false);
    if(file.handle != null)
    {
        s64 file_size = os_file_get_size(&file);
        u8 *data = c_arena_push_size(arena, sizeof(u8) * file_size);

        os_file_read(&file, data, file_size);
        os_file_close(&file);

        result.data  = data;
        result.count = file_size;
    }
    return(result);
}

internal string_t
c_file_read_za(zone_allocator_t *zone, string_t filepath, za_allocation_tag_t tag)
{
    string_t result = {};
    
    file_t file = os_file_open(filepath, false, false, false);
    if(file.handle != null)
    {
        s64 file_size = os_file_get_size(&file);
        u8 *data = c_za_alloc(zone, sizeof(u8) * file_size, tag);

        os_file_read(&file, data, file_size);
        os_file_close(&file);

        result.data  = data;
        result.count = file_size;
    }
    return(result);
}

internal void
c_file_write(string_t filepath, void *data, s64 bytes_to_write, bool8 overwrite)
{
    file_t file = os_file_open(filepath, true, overwrite, false);
    if(file.handle != null)
    {
        os_file_write(&file, data, bytes_to_write);
    }
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
