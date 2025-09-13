#if !defined(OS_LINUX_H)
/* ========================================================================
   $File: os_linux.h $
   $Date: Wed, 27 Aug 25: 07:43PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define OS_LINUX_H
#include "c_types.h"
#include "c_base.h"
#include "c_hash_table.h"

typedef int os_handle_t;

typedef struct os_file_check_event_data
{
    s32         file_data;
    u32         last_move_cookie;
    os_handle_t inotify_handle;

    string_t    filename;
    string_t    old_filename;
}os_file_check_event_data_t;

typedef struct file_watcher_watch_data
{
    s32          inotify_instance;
    void        *inotify_data;
    s64          inotify_bytes_read;
    s64          inotify_cursor;

    os_file_check_event_data_t *directory_data[256];
    u32                         directory_data_count;
}file_watcher_os_watch_data_t;

#define PLATFORM_THREAD_PROC(name) int name(void *user_data)
typedef PLATFORM_THREAD_PROC(thread_proc_t);

int os_work_queue_entry_proc(void *lpParam);
#endif
