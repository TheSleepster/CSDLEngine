#if !defined(DEBUG_CORE_H)
/* ========================================================================
   $File: DEBUG_core.h $
   $Date: Sun, 07 Sep 25: 02:32PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define DEBUG_CORE_H
#define MAX_DEBUG_COUNTERS  (1024)
#define MAX_DEBUG_SNAPSHOTS (144)
#define MAX_DEBUG_EVENTS    (8192 * 16)

#define DEBUG_TIMED_BLOCK()                                             \
    static u32 DEBUG_record_index = (u32)-1;                            \
    if(DEBUG_record_index == (u32)-1)                                   \
        DEBUG_record_index = DEBUG_register_performance_counter(__FILE__, __FUNCTION__, __LINE__); \
                                                                        \
    if(DEBUG_global_state->is_collecting)                               \
        new_timed_block_t block_timer_##__LINE__(DEBUG_record_index);       

struct DEBUG_snapshot_data_t
{
    u64 hit_count;
    u64 cycle_count;
};

struct DEBUG_record_t
{
    char                 *filename;
    char                 *block_name;

    u32                   line_number;
    DEBUG_snapshot_data_t snapshots[MAX_DEBUG_SNAPSHOTS];
};

enum DEBUG_event_type_t
{
    DEBUG_EVENT_INVALID,
    DEBUG_EVENT_TIMER_BEGIN,
    DEBUG_EVENT_TIMER_END,
    DEBUG_EVENT_FRAME_END,
    DEBUG_EVENT_RELOAD_DLL,
    DEBUG_EVENT_SECTION_MARK,
    DEBUG_EVENT_COUNTER
};

struct DEBUG_event_t
{
    u8  event_type;
    u64 cycle_counter;
    u32 thread_id;
    u32 record_index;
};

typedef struct DEBUG_state_data
{
    memory_arena_t  DEBUG_arena;
    u64             cpu_freq;
    
    volatile u32    next_debug_record_entry_index;
    volatile u32    next_debug_event_index;
    volatile u32    event_array_index;
    volatile u32    snapshot_index;
    volatile u32    last_snapshot_index;

    bool8           should_reload_dll;
    bool8           is_collecting;
    bool8           overlay_active;

    DEBUG_record_t  record_array[MAX_DEBUG_COUNTERS];
    DEBUG_event_t   event_array[2][MAX_DEBUG_EVENTS];

    render_group_t *debug_render_group;
}DEBUG_state_data_t;

internal DEBUG_state_data_t *DEBUG_create_debug_state();
internal true_inline void    DEBUG_record_event(u32 record_index, u8 type);
internal true_inline void    DEBUG_set_event_marker(u8 type);
internal u32                 DEBUG_register_performance_counter(char *filename, char *block_name, u32 line_number);
internal void                DEBUG_handle_events();
internal void                DEBUG_display_record_data(asset_manager_t *asset_manager, render_state_t *render_state, asset_handle_t font, float32 delta_time);

struct new_timed_block_t
{
    u32 m_record_index;
    new_timed_block_t(u32 record_index)
    {
        m_record_index = record_index;
        DEBUG_record_event(record_index, DEBUG_EVENT_TIMER_BEGIN);
    }

   ~new_timed_block_t()
    {
        DEBUG_record_event(m_record_index, DEBUG_EVENT_TIMER_END);
    }
};

global DEBUG_state_data_t *DEBUG_global_state;
#endif
