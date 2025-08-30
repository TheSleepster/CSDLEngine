#if !defined(OS_WINDOWS_H)
/* ========================================================================
   $File: os_windows.h $
   $Date: Wed, 27 Aug 25: 07:43PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define OS_WINDOWS_H
#include "c_types.h"
#include "c_string.h"
<<<<<<< HEAD
=======

#define NO_MIN_MAX
#include <windows.h>

typedef void* os_handle_t;

typedef struct os_file_check_event_data
{
    string_t   filename;
    s32        bytes_returned;
    OVERLAPPED overlapped_data;
    HANDLE     file_handle;
    void      *notify_data;

    bool8      read_failed;
}os_file_check_event_data_t;

typedef struct file_watcher_watch_data
{
    os_file_check_event_data *directory_data[256];
    u32                       directory_data_count;
}file_watcher_os_watch_data_t;
>>>>>>> 418a07df52fb204379925e56d5c8db80ebd2b720
#endif
