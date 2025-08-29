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
    FWC_EVENT_SCAN_CHILDREN    = 1 << 6,
    FWC_EVENT_ALL              = FWC_EVENT_ADDED|FWC_EVENT_MODIFIED|FWC_EVENT_DELETED|FWC_EVENT_MOVED|FWC_EVENT_ATTRIBUTE_CHANGE|FWC_EVENT_SCAN_CHILDREN,
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
internal        file_watcher_t  c_file_watcher_create(file_watcher_change_event_t events_to_monitor, bool8 recursive, file_watcher_callback_t *callback, void *user_data);
internal inline void            c_file_watcher_add_path(file_watcher_t *watcher, string_t filepath);
internal inline void            c_file_watcher_issue_check_for_single_path(file_watcher_t *watcher, os_file_check_event_data_t *watch_data);
internal        void            c_file_watcher_issue_check_over_all_paths(file_watcher_t *watcher);

#endif
