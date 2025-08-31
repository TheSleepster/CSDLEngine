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

typedef struct file_watcher_watch_data
{
    s32          inotify_instance;
    void        *inotify_data;
    s64          inotify_bytes_read;
    s64          inotify_cursor;

    hash_table_t directory_table;
}file_watcher_os_watch_data_t;

#define PLATFORM_THREAD_PROC(name) void *name(void *user_data)
typedef PLATFORM_THREAD_PROC(thread_proc_t);

void *os_work_queue_entry_proc(void *user_data);
#endif
