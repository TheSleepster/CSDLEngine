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

typedef void* os_handle_t;

typedef struct windows_directory_data
{
    string_t   filename;
    s32        bytes_returned;
    OVERLAPPED overlapped_data;
    HANDLE     event_handle;

    bool8      read_failed;
}windows_directory_data_t;

typedef struct file_watcher_watch_data
{
    dynamic_array_t directory_data;
}file_watcher_os_watch_data_t;
#endif
