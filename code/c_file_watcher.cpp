/* ========================================================================
   $File: c_file_watcher.cpp $
   $Date: Thu, 28 Aug 25: 12:27PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "c_file_watcher.h"

internal file_watcher_t 
c_file_watcher_create(file_watcher_change_event_t events_to_monitor,
                      bool8                       recursive,
                      file_watcher_callback_t    *callback,
                      void                       *user_data)
{
    file_watcher_t result = {};
    
    result.callback          = callback;
    result.events_to_monitor = events_to_monitor;
    result.user_data         = user_data;
    result.watch_recursively = recursive;
    result.watcher_arena     = c_arena_create(MB(10));
    result.is_valid          = true;
    os_file_watcher_init_watch_data(&result.watcher_arena, &result.os_watch_data);

    return(result);
}

internal inline void
c_file_watcher_add_path(file_watcher_t *watcher, string_t filepath)
{
    watcher->paths_to_watch[watcher->paths_watched] = c_string_make_copy(&watcher->watcher_arena, filepath);
    os_file_watcher_add_path(watcher, filepath);
}

internal inline void
c_file_watcher_issue_check_for_single_path(file_watcher_t *watcher, os_file_check_event_data_t *watch_data)
{
    os_file_watcher_issue_check(watcher, watch_data);
}

internal void
c_file_watcher_issue_check_over_all_paths(file_watcher_t *watcher)
{
    for(u32 data_index = 0;
        data_index < watcher->os_watch_data.directory_data_count;
        ++data_index)
    {
        os_file_check_event_data_t *watch_data = watcher->os_watch_data.directory_data[data_index];
        if(watch_data)
        {
            os_file_watcher_issue_check(watcher, watch_data);
        }
    }
}

internal inline void
c_file_watcher_process_changes(file_watcher_t *watcher)
{
    os_file_watcher_process_changes(watcher, null);
}
