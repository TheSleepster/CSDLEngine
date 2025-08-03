/* ========================================================================
   $File: c_file_api.c $
   $Date: Fri, 25 Jul 25: 01:25PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

// NOTE(Sleepster): Errors from these calls are handled internally 
internal string_t
c_read_entire_file(string_t filepath)
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
c_read_entire_file_arena(memory_arena_t *arena, string_t filepath)
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
c_read_entire_file_za(zone_allocator_t *zone, string_t filepath, za_allocation_tag_t tag)
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
c_write_entire_file(string_t filepath, void *data, s64 bytes_to_write, bool8 overwrite)
{
    file_t file = os_file_open(filepath, true, overwrite, false);
    if(file.handle != null)
    {
        os_file_write(&file, data, bytes_to_write);
    }
}
