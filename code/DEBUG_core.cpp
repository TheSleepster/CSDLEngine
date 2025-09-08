/* ========================================================================
   $File: DEBUG_core.cpp $
   $Date: Mon, 01 Sep 25: 01:07PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "c_base.h"
#include "c_types.h"
#include "c_log_assert.h"

#include "DEBUG_core.h"

internal DEBUG_state_data_t*
DEBUG_create_debug_state()
{
    DEBUG_state_data_t *result = c_arena_bootstrap_allocate_struct(DEBUG_state_data_t, DEBUG_arena, MB(100));
    Assert(result != null);
    ZeroStruct(*result);

    // TODO(Sleepster): Tune the cpu cycle -> ms conversion here. 

    return(result);
}

internal inline void
DEBUG_record_event(u32 record_index, u8 type)
{
    u32 event_index = AtomicIncrement(&DEBUG_global_state->next_debug_event_index);
    Assert(event_index < MAX_DEBUG_EVENTS);

    DEBUG_event_t *event = DEBUG_global_state->event_array[DEBUG_global_state->event_array_index] + event_index;
    event->event_type    = type;
    event->record_index  = record_index;
    event->thread_id     = GetThreadID();

    u32 processor_id = 0;
    event->cycle_counter = rdtscp(&processor_id);
}

internal inline void
DEBUG_set_event_marker(u8 type)
{
    u32 event_index = AtomicIncrement(&DEBUG_global_state->next_debug_event_index);
    DEBUG_event_t *event = DEBUG_global_state->event_array[DEBUG_global_state->event_array_index] + event_index;
    event->event_type    = type;
}

internal u32 
DEBUG_register_performance_counter(char *filename, char *block_name, u32 line_number)
{
    u32 result = AtomicIncrement(&DEBUG_global_state->next_debug_record_entry_index);

    DEBUG_record_t *record = DEBUG_global_state->record_array + result;
    record->filename    = filename;
    record->block_name  = block_name;
    record->line_number = line_number;

    return(result);
}

internal void
DEBUG_handle_events()
{
    DEBUG_event_t *event_array = DEBUG_global_state->event_array[DEBUG_global_state->event_array_index];
    for(u32 record_index = 0;
        record_index < DEBUG_global_state->next_debug_record_entry_index;
        ++record_index)
    {
        DEBUG_record_t *record = DEBUG_global_state->record_array + record_index;
        record->snapshots[DEBUG_global_state->snapshot_index].hit_count   = 0;
        record->snapshots[DEBUG_global_state->snapshot_index].cycle_count = 0;
    }

    u32 event_array_index = !DEBUG_global_state->event_array_index;
    AtomicExchange32(&DEBUG_global_state->event_array_index, event_array_index);

    u32 event_array_count = AtomicExchange32(&DEBUG_global_state->next_debug_event_index, 0);
    for(u32 event_index = 0;
        event_index < event_array_count;
        ++event_index)
    {
        DEBUG_event_t  *event  = event_array + event_index;
        DEBUG_record_t *record = DEBUG_global_state->record_array + event->record_index;
        
        switch(event->event_type)
        {
            case DEBUG_EVENT_TIMER_BEGIN:
            {
                record->snapshots[DEBUG_global_state->snapshot_index].hit_count   += 1;
                record->snapshots[DEBUG_global_state->snapshot_index].cycle_count -= event->cycle_counter;
            }break;
            case DEBUG_EVENT_TIMER_END:
            {
                record->snapshots[DEBUG_global_state->snapshot_index].cycle_count += event->cycle_counter;
            }break;
            case DEBUG_EVENT_FRAME_END:
            {
                u32 next_snapshot_index = (DEBUG_global_state->snapshot_index + 1) % MAX_DEBUG_SNAPSHOTS;
                DEBUG_global_state->last_snapshot_index = AtomicExchange(&DEBUG_global_state->snapshot_index, next_snapshot_index);
            }break;
            case DEBUG_EVENT_RELOAD_DLL:
            {
                DEBUG_global_state->should_reload_dll = true;
            }break;
            default: {}break;
        }
    }
}

