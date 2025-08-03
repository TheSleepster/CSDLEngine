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
    }

    return(result);
}

internal void
os_file_close(file_t *file_data)
{
    CloseHandle(file_data->handle);
    file_data->handle = null;
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
    }

    result = file_size.QuadPart;
    return(result);
}

internal void
os_file_read(file_t *file_data, void *memory, usize bytes_to_read)
{
    BOOL success = ReadFile(file_data->handle, memory, bytes_to_read, 0, 0);
    if(!success)
    {
        log_error("Error reading the file: '%s', message: '%s'...\n",
                  C_STR(file_data->filepath), GetLastError());
    }
}

// NOTE(Sleepster): This is blocking... It will block until the buffer has written everything 
internal void
os_file_write(file_t *file_data, void *memory, usize bytes_to_write)
{
    u32 written = 0;
    BOOL success = WriteFile(file_data->handle, memory, bytes_to_write, &written, null);
    if(!success)
    {
        HRESULT error = HRESULT_FROM_WIN32(GetLastError());
        if(error != ERROR_IO_PENDING)
        {
            log_error("Failed to write '%d' bytes to file '%s'", bytes_to_write, file_data->file_name);
        }
    }
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
    
    HANDLE file_handle = CreateFileA(C_STR(filepath), 0,
                                     FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                     null,
                                     FILE_ATTRIBUTE_READONLY|FILE_FLAG_BACKUP_SEMANTICS,
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

        CloseHandle(file_handle);
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

// TODO(Sleepster): TEST THIS 
internal void
os_directory_visit(string_t filepath, visit_file_data_t visit_file_data)
{
    WIN32_FIND_DATA find_data   = {};
    HANDLE          find_handle = INVALID_HANDLE_VALUE;

    char wildcard_name[256];
    sprintf(wildcard_name, "%s/*", C_STR(filepath));

    string_t dir_name;
    dir_name.count = c_string_length(&wildcard_name);
    dir_name.data  = &wildcard_name;


    find_handle = FindFirstFile(wildcard_name, &find_data);
    if(find_handle == INVALID_HANDLE_VALUE)
    {
        log_error("Filepath: '%s' is invalid... returning...\n", C_STR(filepath));
        return;
    }

    u32 cursor = 0;
    dynamic_array_t directories = c_dynamic_array_create(string_t, 10);
    c_dynamic_array_append_value(&directories, &dir_name);

    while(cursor < directories.indices_used)
    {
        do
        {
            bool8 is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if(is_directory)
            {
                if(strcmp(find_data.cFileName, ".") != 0 || strcmp(find_data.cFileName, "..") != 0)
                {
                    if(visit_file_data.function != null) visit_file_data.function(&visit_file_data, visit_file_data.user_data);

                    if(visit_file_data.recursive)
                    {
                        char *name = find_data.cFileName;
                        visit_file_data.file_name = STR(name);
                        visit_file_data.full_name = c_string_concat(&global_context.temporary_arena, filepath, STR(name));

                        c_dynamic_array_append_value(&directories, &visit_file_data.full_name);
                    }
                }
            }
            else
            {
                if(visit_file_data.function != null) visit_file_data.function(&visit_file_data, visit_file_data.user_data);
            }
        }while(FindNextFile(find_handle, &find_data));
    }

    CloseHandle(find_handle);
    c_dynamic_array_destroy(&directories);
}

