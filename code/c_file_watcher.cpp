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
    os_file_watcher_init_watch_data(&result.watcher_arena, &result.os_watch_data);

    return(result);
}

internal void
c_file_watcher_add_path(file_watcher_t *watcher, string_t filepath)
{
    c_dynamic_array_append_value(&watcher->paths_to_watch,
                                 c_string_make_copy(&watcher->watcher_arena, filepath));
    os_file_watcher_add_path(watcher, filepath);
}
