#if !defined(OS_PLATFORM_FILE_H)
/* ========================================================================
   $File: os_platform_file.h $
   $Date: Mon, 28 Jul 25: 07:44PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define OS_PLATFORM_FILE_H
#include "c_base.h"
#include "c_types.h"
#include "c_debug.h"
#include "c_memory.h"
#include "c_string.h"

#include "c_file_api.h"

/////////////////
// MEMORY
/////////////////

internal void* os_allocate_memory(usize allocation_size);
internal void  os_free_memory(void *data, usize free_size);

//////////////////
// FILE IO STUFF
//////////////////

internal file_t        os_file_open(string_t filepath, bool8 for_writing, bool8 overwrite, bool8 overlapping_io);
internal void          os_file_close(file_t *file_data);
internal s64           os_file_get_size(file_t *file_data);
internal void          os_file_read(file_t *file_data, void *memory, usize bytes_to_read);
internal void          os_file_write(file_t *file_data, void *memory, usize bytes_to_write);

internal mapped_file_t os_file_map(string_t filepath);
internal bool8         os_file_unmap(mapped_file_t *map_data);

internal bool8         os_file_exists(string_t filepath);
internal file_data_t   os_file_get_modtime_and_size(string_t filepath);
internal bool8         os_file_replace_or_rename(string_t old_file, string_t new_file);

internal bool8         os_directory_exists(string_t filepath);
internal void          os_directory_visit(string_t filepath, visit_file_data_t *visit_file_data);


#if defined OS_WINDOWS
# include "os_windows.c"

#elif defined OS_LINUX
# include "os_linux.c"

#elif defined OS_MAC
# error "lmao really?"
#endif

#endif
