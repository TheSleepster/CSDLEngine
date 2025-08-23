/* ========================================================================
   $File: os_linux.c $
   $Date: Wed, 30 Jul 25: 12:27PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include "c_types.h"
#include "c_base.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h> 
#include <unistd.h>
#include <string.h> 

internal inline void*
os_allocate_memory(usize allocation_size)
{
    errno = 0;
    
    void *data = mmap(0, allocation_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(errno == MAP_FAILED)
    {
        int error = errno;
        log_fatal("mmap failed... error: (%s), code: '%d'...\n", strerror(error), error);

        data = null;
    }

    return(data);
}

internal inline void
os_free_memory(void *data, usize free_size)
{
    if(munmap(data, free_size) == -1)
    {
        int error = errno;
        log_fatal("munmap failed... error: (%s), code: '%d'...\n", strerror(error), error);
    }
}

//////////////////////
// FILE IO STUFF
/////////////////////

internal file_t
os_file_open(string_t filepath, bool8 for_writing, bool8 overwrite, bool8 overlapping_io)
{
    file_t result = {};
    result.file_name = c_string_get_filename_from_path(filepath);
    result.filepath  = filepath;

    s32 flags = 0;
    if(for_writing)
    {
        result.for_writing = true;

        if(overwrite)
        {
            flags = O_RDWR|O_CREAT;
        }
        else
        {
            flags = O_RDWR|O_CREAT|O_TRUNC;
        }
    }
    else
    {
        flags = O_RDONLY;
    }

    if(overlapping_io)
    {
        result.overlapping = true;
        flags |= O_NONBLOCK;
    }

    result.handle = open(filepath.data, flags, 0666);
    if(result.handle == -1)
    {
        log_error("Failure to open file '%s' for %s, error: '%s'...\n",
                  filepath.data, for_writing ? "writing" : "reading",
                  strerror(errno));

        ZeroStruct(result);
        result.handle = -1;
    }

    return(result);
}

internal bool8
os_file_close(file_t *file_data)
{
    bool8 result = false;
    
    Assert(file_data->handle != -1);
    if(close(file_data->handle) == 0)
    {
        file_data->handle = -1;
        result = true;
    }

    return(result);
}

internal s64
os_file_get_size(file_t *file_data)
{
    s64 file_size = 0;
    
    struct stat file_stats = {};
    if(fstat(file_data->handle, &file_stats) != -1)
    {
        file_size = file_stats.st_size;
    }
    else
    {
        log_error("Failure to get the file size for file: '%s', error: '%s'...\n",
                  file_data->filepath, strerror(errno));
    }

    return(file_size);
}

internal bool8
os_file_read(file_t *file_data, void *memory, u32 file_offset, u32 bytes_to_read)
{
    bool8 result = false;

    usize bytes_read = 0; 
    if(lseek(file_data->handle, file_offset, SEEK_SET) != -1)
    {
        bytes_read = read(file_data->handle, memory, bytes_to_read);
        if(bytes_read  == bytes_to_read)
        {
            result = true;
        }
    }

    if(bytes_read == 0)
    {
        log_error("Failure to read file '%s', error: '%s'...\n", file_data->filepath, strerror(errno));
    }

    return(result);
}

internal bool8
os_file_write(file_t *file_data, void *memory, usize bytes_to_write)
{
    bool8 result = false;
    
    usize bytes_written = write(file_data->handle, memory, bytes_to_write);
    if(bytes_written == bytes_to_write)
    {
        result = true;
    }
    else
    {
        log_error("Failure to write file '%s', error: '%s'...\n", file_data->filepath, strerror(errno));
    }

    return(result);
}

internal mapped_file_t
os_file_map(string_t filepath)
{
    mapped_file_t result = {};

    result.file = os_file_open(filepath, false, false, false);
    if(result.file.handle >= 0)
    {
        s64 file_size = os_file_get_size(&result.file);
        if(file_size == 0)
        {
            return(result);
        }

        void *mapped_data = mmap(null, file_size, PROT_READ, MAP_PRIVATE, result.file.handle, 0);
        if(mapped_data == MAP_FAILED)
        {
            log_error("MMAP failed to map the data for file: '%s'... error: '%s'...\n", result.file.filepath, strerror(errno));
        }

        result.mapped_file_data.data  = mapped_data;
        result.mapped_file_data.count = file_size;
    }

    return(result);
}

internal bool8
os_file_unmap(mapped_file_t *map_data)
{
    bool8 result = false;
    
    Assert(map_data->mapped_file_data.data  != null);
    Assert(map_data->mapped_file_data.count != 0);

    if(munmap(map_data->mapped_file_data.data, map_data->mapped_file_data.count) == 0)
    {
        result = true;
    }
    else
    {
        log_error("Failure to unmap file: '%s'... error: '%s'...\n", map_data->file.filepath, strerror(errno));
    }

    return(result);
}

internal bool8
os_file_exists(string_t filepath)
{
    bool8 result = false;

    struct stat file_stats;
    result = (stat(filepath.data, &file_stats) == 0);

    return(result);
}

internal file_data_t
os_file_get_modtime_and_size(string_t filepath)
{
    file_data_t result = {};
    struct stat file_stats;
    if(stat(filepath.data, &file_stats) == 0)
    {
        result.file_size    = file_stats.st_size;
        result.last_modtime = file_stats.st_mtime;
        result.filepath     = filepath;
        result.filename     = c_string_get_filename_from_path(filepath);
    }
    else
    {
        log_error("Failure to get information about file '%s'... error: '%s'\n", filepath.data, strerror(errno));
    }

    return(result);
}

internal bool8
os_file_replace_or_rename(string_t old_file, string_t new_file)
{
    bool8 result = false;
    if(rename(old_file.data, new_file.data) == 0)
    {
        result = true;
    }
    else
    {
        log_error("Failure to rename file '%s' to that of '%s', error: '%s'...\n",
                  old_file.data, new_file.data, strerror(errno));
    }

    return(result);
}

internal bool8
os_directory_exists(string_t filepath)
{
    bool8 result = false;
    struct stat file_stats;
    if(stat(filepath.data, &file_stats) == 0)
    {
        result = S_ISDIR(file_stats.st_mode);
    }

    return(result);
}

internal void
os_directory_visit(string_t filepath, visit_file_data_t *visit_file_data)
{
    DIR *directory = opendir(filepath.data);
    if(directory != null)
    {
        struct dirent *entry;
        while((entry = readdir(directory)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            visit_file_data->filename = c_string_make_heap(&global_context.temporary_arena, STR(entry->d_name));
            string_t temp_name        = c_string_concat(&global_context.temporary_arena, filepath, STR("/"));
            visit_file_data->fullname = c_string_concat(&global_context.temporary_arena, temp_name, visit_file_data->filename);

            bool8 is_directory            = (entry->d_type == DT_DIR);
            visit_file_data->is_directory = is_directory;

            if(is_directory && visit_file_data->recursive)
            {
                os_directory_visit(visit_file_data->fullname, visit_file_data);
            }
            else if(!is_directory && visit_file_data->function != null)
            {
                visit_file_data->function(visit_file_data, visit_file_data->user_data);
            }
        }
        closedir(directory);
    }
    else
    {
        log_error("Failure to open the directory: '%s'... error of: '%s'...\n",
                  filepath.data, strerror(errno));
    }
}
