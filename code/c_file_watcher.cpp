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
    result.observed_changes  = c_dynamic_array_create(file_watcher_recorded_change_t, 20);
    result.paths_to_watch    = c_dynamic_array_create(string_t, 20);
    result.is_valid          = true;

    return(result);
}

internal void
c_file_watcher_add_path(file_watcher_t *watcher, string_t filepath)
{
    c_dynamic_array_append_value(&watcher->paths_to_watch,
                                 c_string_make_copy(&watcher->watcher_arena, filepath));
}
<<<<<<< HEAD

internal inline void
c_file_watcher_issue_check_for_single_path(file_watcher_t *watcher, os_file_check_event_data_t *watch_data)
{
}

internal void
c_file_watcher_issue_check_over_all_paths(file_watcher_t *watcher)
{
    for(u32 path_index = 0;
        path_index < watcher->paths_to_watch.indices_used;
        ++path_index)
    {
        os_file_check_event_data_t *watch_data = (os_file_check_event_data_t*)c_dynamic_array_get(&watcher->paths_to_watch, path_index);
        if(watch_data)
        {
        }
    }
}

internal inline void
c_file_watcher_process_changes(file_watcher_t *watcher)
{
}
=======
>>>>>>> parent of f5bf99c (rip overcomplicated file watcher)
