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
#include "c_memory_arena.h"
#include "c_string.h"
#include "c_file_watcher.h"

#include "c_multithreading_primitives.h"
#include "c_file_api.h"

/*===========================================
  ============== OS MEMORY API ==============
  ===========================================*/

internal void* os_allocate_memory(usize allocation_size);
internal void  os_free_memory(void *data, usize free_size);
internal void* os_reallocate_memory(void *offset, u64 allocation_size);

/*===========================================
  ============== FILE IO STUFF ==============
  ===========================================*/

internal file_t        os_file_open(string_t filepath, bool8 for_writing, bool8 overwrite, bool8 overlapping_io);
internal bool8         os_file_close(file_t *file_data);
internal s64           os_file_get_size(file_t *file_data);
internal bool8         os_file_read(file_t *file_data, void *memory, u32 file_offset, u32 bytes_to_read);
internal bool8         os_file_write(file_t *file_data, void *memory, usize bytes_to_write);

internal mapped_file_t os_file_map(string_t filepath);
internal bool8         os_file_unmap(mapped_file_t *map_data);

internal bool8         os_file_exists(string_t filepath);
internal file_data_t   os_file_get_modtime_and_size(string_t filepath);
internal bool8         os_file_replace_or_rename(string_t old_file, string_t new_file);

internal bool8         os_directory_exists(string_t filepath);
internal void          os_directory_visit(string_t filepath, visit_file_data_t *visit_file_data);

/*===========================================
  ======= FILE WATCHER OS FUNCTIONS =========
  ===========================================*/
internal void  os_file_watcher_init_watch_data(memory_arena_t *arena, file_watcher_os_watch_data_t *watch_data);
internal bool8 os_file_watcher_add_path(file_watcher_t *watcher, string_t path);
internal void  os_file_watcher_issue_check(file_watcher_t *watcher, os_file_check_event_data_t *directory_data);
internal void  os_file_watcher_process_changes(file_watcher_t *watcher, bool8 *changed);

#if OS_WINDOWS
# include "os_windows.cpp"

#elif OS_LINUX
# include "os_linux.cpp"

#elif OS_MAC
# error "lmao really?"
#endif

#endif
