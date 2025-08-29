#if !defined(C_FILE_WATCHER_H)
/* ========================================================================
   $File: c_file_watcher.h $
   $Date: Thu, 28 Aug 25: 12:27PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_FILE_WATCHER_H
#include "c_base.h"
#include "c_types.h"
#include "c_debug.h"
#include "c_memory.h"
#include "c_string.h"
#include "c_array.h"
#include "c_hash_table.h"

#if OS_WINDOWS
#include "os_windows.h"
#elif OS_LINUX|OS_MAC
#include "os_linux.h"
#endif

/*===========================================
  =============== FILE WATCHER ==============
  ===========================================*/
typedef struct file_watcher              file_watcher_t;

#define FILE_WATCHER_CALLBACK(name) void name(file_watcher_t *watcher, s32 event, void *user_data)
typedef FILE_WATCHER_CALLBACK(file_watcher_callback_t);

typedef enum file_watcher_change_event
{
    FWC_EVENT_NONE             = 1 << 0,
    FWC_EVENT_ADDED            = 1 << 1,
    FWC_EVENT_MODIFIED         = 1 << 2,
    FWC_EVENT_DELETED          = 1 << 3,
    FWC_EVENT_MOVED            = 1 << 4,
    FWC_EVENT_ATTRIBUTE_CHANGE = 1 << 5,
    FWC_EVENT_ALL              = FWC_EVENT_ADDED|FWC_EVENT_MODIFIED|FWC_EVENT_DELETED|FWC_EVENT_MOVED|FWC_EVENT_ATTRIBUTE_CHANGE,
    WFC_EVENT_COUNT,
}file_watcher_change_event_t;

typedef struct file_watcher_recorded_change
{
    string_t                    full_path;
    file_watcher_change_event_t changes;
    u64                         last_change_timestamp;
}file_watcher_recorded_change_t;

typedef struct file_watcher
{
    // NOTE(Sleepster): this has an arena mainly for copy string.
    memory_arena_t               watcher_arena;
    bool8                        is_valid;
    
    file_watcher_callback_t     *callback;
    file_watcher_change_event    events_to_monitor;
    void                        *user_data;

    bool8                        watch_recursively;
    dynamic_array_t              observed_changes; 
    dynamic_array_t              paths_to_watch;

    u32                          notify_buffer_size;
    file_watcher_os_watch_data_t os_watch_data;

    bool8                        issues_when_checking;
}file_watcher_t;

/*===========================================
  ============ API DEFINITIONS ==============
  ===========================================*/
internal void  os_file_watcher_init_watch_data(memory_arena_t *arena, file_watcher_os_watch_data_t *watch_data);
internal bool8 os_file_watcher_add_path(file_watcher_t *watcher, string_t path);
internal void  os_file_watcher_issue_async_update(file_watcher_t *watcher, windows_directory_data_t *directory_data);

#endif
