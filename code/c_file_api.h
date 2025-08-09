#if !defined(C_FILE_API_H)
/* ========================================================================
   $File: c_file_api.h $
   $Date: Fri, 25 Jul 25: 01:24PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_FILE_API_H
#include "c_types.h"
#include "c_string.h"

// TODO(Sleepster): THREAD SAFE OVERLAPPING IO 

// NOTE(Sleepster): This is stupid... but C doens't make this easier. 
#if defined OS_WINDOWS
typedef void* file_handle_t;
#elif defined OS_LINUX | defined OS_MAC
typedef int    file_handle_t
#endif

typedef struct file
{
    file_handle_t handle;
    string_t      file_name;
    string_t      filepath;

    bool8         overlapping;
    bool8         for_writing;
}file_t;

typedef struct mapped_file_data
{
    file_t        file;
    file_handle_t mapping_handle;
    string_t      mapped_file_data;
}mapped_file_t;

typedef struct file_data
{
    u64      last_modtime;
    u64      file_size;

    string_t filename;
    string_t filepath;
}file_data_t;

struct visit_file_data;

#define VISIT_FILES(name) void name(struct visit_file_data *visit_file_data, void *user_data)
typedef VISIT_FILES(visit_files_pfn_t);

typedef struct visit_file_data
{
    visit_files_pfn_t *function;
    void              *user_data;

    string_t           filename;
    string_t           fullname;

    bool8              recursive;
    bool8              is_directory;
}visit_file_data_t;

/* TODO(Sleepster): c_file_write_overlapped()
 *
 * We just generally need async fileIO...
 *
 * We might want to be able to supply an offset to c_write_file_*
 * functions for specific operating systems.  Windows keeps a write
 * pointer for each file you create with CreateFile... But I don't
 * know how Linux or Mac does it...
 */

internal inline file_t            c_file_open(string_t filepath, bool8 create);
internal inline bool8             c_file_close(file_t *file);
internal        string_t          c_file_read(string_t filepath);
internal        string_t          c_file_read_arena(memory_arena_t *arena, string_t filepath);
internal        string_t          c_file_read_za(zone_allocator_t *zone, string_t filepath, za_allocation_tag_t tag);
internal inline bool8             c_file_open_and_write(string_t filepath, void *data, s64 bytes_to_write, bool8 overwrite);
internal inline bool8             c_file_write(file_t *file, void *data, s64 bytes_to_write);
internal inline bool8             c_file_write_string(file_t *file, string_t data);

internal        s64               c_file_get_size(string_t filepath);
internal inline file_data_t       c_file_get_data(string_t filepath);
internal inline bool8             c_file_replace_or_rename(string_t old_file, string_t new_file);
internal inline mapped_file_t     c_file_map(string_t filepath);
internal inline bool8             c_file_unmap(mapped_file_t *map_data);
    
internal inline bool8             c_directory_exists(string_t filepath);

internal        visit_file_data_t c_directory_create_visit_data(visit_files_pfn_t *function, bool8 recursive, void *user_data);
internal inline void              c_directory_visit(string_t filepath, visit_file_data_t *visit_file_data);
#endif
