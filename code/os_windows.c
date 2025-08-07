/* ========================================================================
   $File: os_windows.c $
   $Date: Mon, 28 Jul 25: 07:47PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define WIN32_LEAN_AND_MEAN
#define NO_MIN_MAX
#include <windows.h>

#include "c_types.h"
#include "c_base.h"

// TODO(Sleepster): UNICODE 

/////////////////////
// MEMORY FUNCTIONS
/////////////////////

internal inline void*
os_allocate_memory(usize allocation_size)
{
    return(VirtualAlloc(0, allocation_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE));
}

internal inline void
os_free_memory(void *data, usize free_size)
{
    VirtualFree(data, 0, MEM_RELEASE);
}

///////////////////////////////////
// PLATFORM FILE IO FUNCTIONS
///////////////////////////////////

// TODO(Sleepster): UNICODE CreateFileW 
internal file_t
os_file_open(string_t filepath, bool8 for_writing, bool8 overwrite, bool8 overlapping_io)
{
    file_t result = {};
    result.file_name = c_get_filename_from_path(filepath);
    result.filepath  = filepath;

    u32 overlapping = 0;
    if(overlapping_io)
    {
        overlapping        = FILE_FLAG_OVERLAPPED;
        result.overlapping = true;
    }

    if(for_writing)
    {
        u32 mode = 0;
        if(overwrite) mode = OPEN_ALWAYS;
        else mode = CREATE_ALWAYS;

        result.for_writing = true;

        result.handle = CreateFileA(C_STR(filepath),
                                    FILE_GENERIC_READ|FILE_GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    0,
                                    mode,
                                    overlapping,
                                    0);
    }
    else
    {
        result.handle = CreateFileA(C_STR(filepath),
                                    FILE_GENERIC_READ,
                                    FILE_SHARE_READ,
                                    0,
                                    OPEN_ALWAYS,
                                    0,
                                    0);
    }

    if(result.handle == INVALID_HANDLE_VALUE)
    {
        log_error("Failed to open file: '%s' for reading... error: '%s'...\n",
                  C_STR(filepath), GetLastError());

        ZeroStruct(result);
    }

    return(result);
}

internal bool8 
os_file_close(file_t *file_data)
{
    bool8 result = CloseHandle(file_data->handle);
    if(result)
    {
        file_data->handle = null;
    }

    return(result);
}

internal s64
os_file_get_size(file_t *file_data)
{
    s64 result = 0;
    
    LARGE_INTEGER file_size;
    DWORD success = GetFileSizeEx(file_data->handle, &file_size);
    if(success == 0)
    {
        log_error("failed to get file size for file: '%s\n', error: '%s'...\n",
                  C_STR(file_data->file_name), GetLastError());
        result = 0;
    }

    result = file_size.QuadPart;
    return(result);
}

internal bool8 
os_file_read(file_t *file_data, void *memory, usize bytes_to_read)
{
    bool8 result = true;
    
    BOOL success = ReadFile(file_data->handle, memory, bytes_to_read, 0, 0);
    if(!success)
    {
        log_error("Error reading the file: '%s', message: '%s'...\n",
                  C_STR(file_data->filepath), GetLastError());
        result = false;
    }

    return(result);
}

// NOTE(Sleepster): This is blocking... It will block until the buffer has written everything 
internal bool8 
os_file_write(file_t *file_data, void *memory, usize bytes_to_write)
{
    bool8 result = true;
    
    u32 written = 0;
    BOOL success = WriteFile(file_data->handle, memory, bytes_to_write, &written, null);
    if(!success)
    {
        HRESULT error = HRESULT_FROM_WIN32(GetLastError());
        if(error != ERROR_IO_PENDING)
        {
            log_error("Failed to write '%d' bytes to file '%s'", bytes_to_write, file_data->file_name);
        }

        result = false;
    }

    return(result);
}

internal mapped_file_t
os_file_map(string_t filepath)
{
    mapped_file_t result = {};
    result.file = os_file_open(filepath, false, false, false);
    if(result.file.handle != null)
    {
        s64 size = os_file_get_size(&result.file);
        if(size == 0) return result;

        result.mapping_handle = CreateFileMappingW(result.mapping_handle, null, PAGE_READONLY, 0, 0, null);
        if(result.mapping_handle && result.file.handle != null)
        {
            void *data = MapViewOfFile(result.mapping_handle, FILE_MAP_READ, 0, 0, size);
            if(data)
            {
                result.mapped_file_data.data  = data;
                result.mapped_file_data.count = size;
            }
        }
    }

    
    DWORD error      = GetLastError();
    DWORD error_code = HRESULT_FROM_WIN32(ERROR);
    if(ERROR != 0)
    {
        log_error("Failed to create file mapping and view... error code: '%d', error message: '%s'...\n", error, error_code);
    }

    return(result);
}

internal bool8 
os_file_unmap(mapped_file_t *map_data)
{
    bool8 result = true;
    if(map_data->file.handle != INVALID_HANDLE_VALUE)
    {
        if(map_data->mapping_handle)
        {
            if(map_data->mapped_file_data.data)
            {
                UnmapViewOfFile(map_data->mapped_file_data.data);
                map_data->mapped_file_data.data  = null;
                map_data->mapped_file_data.count = 0;
            }

            CloseHandle(map_data->mapping_handle);
            map_data->mapping_handle = null;
        }

        os_file_close(map_data->file.handle);
        map_data->file.handle = INVALID_HANDLE_VALUE;
    }
    else
    {
        result = false;
        log_warning("Failed to unmap file... map_data->file.handle is invalid...\n");
    }

    return(result);
}

internal inline bool8
os_file_exists(string_t filepath)
{
    bool8 result = false;
    result = GetFileAttributes(C_STR(filepath)) != INVALID_FILE_ATTRIBUTES;

    return(result);
}

internal file_data_t
os_file_get_modtime_and_size(string_t filepath)
{
    file_data_t result = {};
    
    HANDLE file_handle = CreateFileA(C_STR(filepath),
                                     FILE_GENERIC_READ,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                     null,
                                     OPEN_ALWAYS,
                                     0,
                                     0);
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        FILETIME write_time;
        GetFileTime(file_handle, null, null, &write_time);

        ULARGE_INTEGER filetime;
        memcpy(&write_time, &filetime, sizeof(FILETIME));

        LARGE_INTEGER file_size;
        GetFileSize(file_handle, &file_size);

        result.file_size = file_size.QuadPart;
        result.last_modtime = filetime.QuadPart;
        result.filepath     = filepath;
        result.filename     = c_get_filename_from_path(filepath);

        CloseHandle(file_handle);
    }
    else
    {
        log_error("Cannot get data for file: '%s'...\n", C_STR(filepath));
    }

    return(result);
}

internal bool8
os_file_replace_or_rename(string_t old_file, string_t new_file)
{
    bool8 result = true;
    
    DWORD attibutes = GetFileAttributes(C_STR(new_file));

    BOOL success = false;
    if(attibutes == INVALID_FILE_ATTRIBUTES)
    {
        success = MoveFile(C_STR(old_file), C_STR(new_file));
    }
    else
    {
        success = ReplaceFile(C_STR(new_file), C_STR(old_file), null, 0, null, null);
    }

    if(!success)
    {
        DWORD error_code = GetLastError();
        log_error("Failed to move file '%s' to '%s'... error code: %d\n",
                  C_STR(old_file), C_STR(new_file), HRESULT_FROM_WIN32(error_code));
        result = false;
    }

    return(result);
}

internal bool8
os_directory_exists(string_t filepath)
{
    bool8 result = false;
    DWORD attributes = GetFileAttributes(C_STR(filepath));
    if(attributes != INVALID_FILE_ATTRIBUTES)
    {
        result = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    return(result);
}

internal void
os_directory_visit(string_t filepath, visit_file_data_t *visit_file_data)
{
    Assert(visit_file_data);
    
    WIN32_FIND_DATA find_data;
    HANDLE          find_handle = INVALID_HANDLE_VALUE;

    u32 cursor = 0;
    dynamic_array_t directories = c_dynamic_array_create(string_t, 10);
    c_dynamic_array_append(&directories, filepath);

    while(cursor < directories.indices_used)
    {
        string_t *directory_name = c_dynamic_array_get(&directories, cursor);
        cursor += 1;

        char wildcard_name[256];
        sprintf(wildcard_name, "%s/*", directory_name->data);

        string_t dir_name;
        dir_name.count = c_string_length(wildcard_name);
        dir_name.data  = wildcard_name;

        find_handle = FindFirstFileEx(dir_name.data, FindExInfoBasic, &find_data, FindExSearchNameMatch, null, FindExSearchNameMatch);
        if(find_handle == INVALID_HANDLE_VALUE)
        {
            log_error("Filepath: '%s' is invalid... returning...\n", C_STR(filepath));
            return;
        }

        BOOL success = true;
        while(true)
        {
            char *name = find_data.cFileName;
            visit_file_data->filename = c_string_make_heap(&global_context.temporary_arena, STR(name));
            string_t temp_name         = c_string_concat(&global_context.temporary_arena, *directory_name, STR("/"));
            visit_file_data->fullname = c_string_concat(&global_context.temporary_arena, temp_name, visit_file_data->filename);

            bool8 is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if(is_directory)
            {
                if(strcmp(find_data.cFileName, ".") != 0 && strcmp(find_data.cFileName, "..") != 0)
                {
                    visit_file_data->is_directory = true;
                    if(visit_file_data->recursive)
                    {
                        c_dynamic_array_append(&directories, visit_file_data->fullname);
                    }
                }
            }
            else
            {
                visit_file_data->is_directory = false;
                if(visit_file_data->function != null)
                    visit_file_data->function(visit_file_data, visit_file_data->user_data);
            }
            
            success = FindNextFile(find_handle, &find_data);
            if(!success) break;
        }

        FindClose(find_handle);
    }

    c_dynamic_array_destroy(&directories);
}

