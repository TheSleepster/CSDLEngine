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
internal void  os_file_watcher_add_change_event(file_watcher_t *watcher, os_file_check_event_data_t *watch_data, u32 changes);
internal void  os_file_watcher_process_changes(file_watcher_t *watcher, bool8 *changed);

/*===========================================
  ============== MULTITHREADING =============
  ===========================================*/

typedef struct os_thread
{
    os_handle_t handle;
    u32         thread_id;
    void       *user_data;
}os_thread_t;

typedef struct os_mutex
{
    os_handle_t handle;
}os_mutex_t;

typedef struct os_semaphore
{
    os_handle_t handle;
}os_semaphore_t;
    
#define PLATFORM_THREAD_PROC(name) s32 name(void *user_data)
typedef PLATFORM_THREAD_PROC(thread_proc_t);

internal inline s32            os_get_cpu_count();
internal        os_semaphore_t os_semaphore_create(s32 initial_thread_count, s32 max_thread_count, string_t semaphore_name);
internal inline void           os_semaphore_close(os_semaphore_t *semaphore);
internal inline s32            os_semaphore_release(os_semaphore_t *semaphore, s32 threads_to_release);
internal inline bool8          os_semaphore_destroy(os_semaphore_t *semaphore);
internal        os_thread_t    os_thread_create(thread_proc_t *proc, void *user_data, bool8 close_handle);
internal inline void           os_thread_wait(os_semaphore_t *semaphore, u64 wait_duration_ms);
internal inline bool8          os_thread_close_handle(os_thread_t *thread_data);
internal inline os_mutex_t     os_mutex_create();
internal inline void           os_mutex_free(os_mutex_t *mutex);
internal inline bool8          os_mutex_lock(os_mutex_t *mutex);
internal inline bool8          os_mutex_unlock(os_mutex_t *mutex);

#if OS_WINDOWS
# include "os_windows.cpp"

#elif OS_LINUX
# include "os_linux.cpp"

#elif OS_MAC
# error "lmao really?"
#endif

#endif
