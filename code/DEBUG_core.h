#if !defined(DEBUG_CORE_H)
/* ========================================================================
   $File: DEBUG_core.h $
   $Date: Sun, 07 Sep 25: 02:32PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "r_render_API.h"

#define DEBUG_CORE_H
#define MAX_DEBUG_FRAME_HISTORY  (1024)
#define MAX_DEBUG_COUNTERS       (1024)
#define MAX_DEBUG_SNAPSHOTS      (MAX_DEBUG_FRAME_HISTORY)
#define MAX_DEBUG_EVENTS         (8192 * 4)
#define MAX_THREADS              (32)
#define MAX_DEBUG_FRAME_SECTIONS (MAX_DEBUG_FRAME_HISTORY)

#define DEBUG_TIMED_BLOCK()                                             \
    local_persist u32 DEBUG_record_index = (u32)-1;                     \
    if(DEBUG_record_index == (u32)-1)                                   \
        DEBUG_record_index = DEBUG_register_performance_counter((char *)__FILE__, (char *)__FUNCTION__, __LINE__); \
    new_timed_block_t block_timer_##__LINE__(DEBUG_record_index);        

typedef struct render_group render_group_t;
typedef struct asset_manager asset_manager_t;
typedef struct render_state render_state_t;
typedef struct asset_handle asset_handle_t;

typedef struct DEBUG_snapshot_data
{
    u64 hit_count;
    u64 cycle_count;
}DEBUG_snapshot_data_t;

typedef struct DEBUG_record
{
    char                 *filename;
    char                 *block_name;

    u32                   line_number;
    u32                   record_index;
    DEBUG_snapshot_data_t snapshots[MAX_DEBUG_SNAPSHOTS];
}DEBUG_record_t;

typedef enum DEBUG_event_type
{
    DEBUG_EVENT_INVALID,
    DEBUG_EVENT_TIMER_BEGIN,
    DEBUG_EVENT_TIMER_END,
    DEBUG_EVENT_FRAME_END,
    DEBUG_EVENT_RELOAD_DLL,
    DEBUG_EVENT_SECTION_MARK,
    DEBUG_EVENT_COUNTER
}DEBUG_event_type_t;

typedef struct DEBUG_event
{
    u32   event_type;
    u32   frame_index;
    u32   thread_id;
    u32   record_index;
    u64   cycle_counter;
}DEBUG_event_t;

typedef struct DEBUG_frame_section
{
    DEBUG_record_t *record;
    u64             owner_thread_id;
    float32         min_clocks;
    float32         max_clocks;
    u32             frame_index;
    u32             thread_index;
}DEBUG_frame_section_t;

typedef struct DEBUG_open_block
{
    u32               opened_frame_index;
    DEBUG_event_t    *opening_event;
    DEBUG_open_block *parent_block;
    DEBUG_open_block *next_free_block;
}DEBUG_open_block_t;

typedef struct DEBUG_thread_data
{
    bool8               is_valid;
    u32                 thread_id;
    u32                 thread_index;
    DEBUG_open_block_t *first_open_block;
    DEBUG_open_block_t *first_free_open_block;
}DEBUG_thread_data_t;

typedef struct DEBUG_frame_data
{
    u64                   begin_clock;
    u64                   end_clock;

    u32                   thread_count;
    DEBUG_thread_data_t   thread_data[MAX_THREADS];

    u32                   section_count;
    DEBUG_frame_section   sections[MAX_DEBUG_FRAME_SECTIONS];
}DEBUG_frame_data_t;

typedef struct DEBUG_state_data
{
    memory_arena_t      DEBUG_arena;
    u64                 cpu_freq;
    
    volatile u32        next_debug_record_entry_index;
    volatile u32        next_debug_event_index;
    volatile u32        event_array_index;
    volatile u32        current_frame_index;
    volatile u32        last_frame_index;

    float32             frame_bar_scale;

    bool8               should_reload_dll;
    bool8               is_collecting;
    bool8               overlay_active;

    DEBUG_record_t      record_array[MAX_DEBUG_COUNTERS];
    DEBUG_event_t       event_array[2][MAX_DEBUG_EVENTS];

    DEBUG_frame_data_t  frame_data[MAX_DEBUG_SNAPSHOTS];
    DEBUG_open_block_t *first_free_open_block;
    render_group_t     *debug_render_group;
}DEBUG_state_data_t;



struct DEBUG_event_t
{
    u32   event_type;
    u32   frame_index;
    u32   thread_id;
    u32   record_index;
    u64   cycle_counter;
};

struct debug_scope_data_t
{
    u64   begin_clock;
    u64   end_clock;
};

struct debug_thread_data_t
{
    debug_event_data_t events[MAX_EVENTS];
    debug_scope_data_t scopes[MAX_SCOPES];
};

struct debug_frame_data_t
{
    debug_thread_data_t threads[32];
    DEBUG_record_t      record_array[MAX_DEBUG_COUNTERS];
};

struct debug_state_t
{
    debug_frame_data_t frames[MAX_FRAMES];
};

typedef struct input_controller input_controller_t;
internal void                DEBUG_handle_ui_input(input_controller_t *DEBUG_controller);
internal DEBUG_state_data_t *DEBUG_create_debug_state();
internal true_inline void    DEBUG_record_event(u32 record_index, u8 type);
internal true_inline void    DEBUG_record_allocation_event(u8 type, u64 allocation_size);
internal true_inline void    DEBUG_set_event_marker(u8 type);
internal u32                 DEBUG_register_performance_counter(char *filename, char *block_name, u32 line_number);
internal void                DEBUG_handle_events();
internal vec2_t              DEBUG_display_record_data(asset_manager_t *asset_manager, render_state_t *render_state, asset_handle_t font, float32 delta_time);
internal void                DEBUG_render_section_graph(asset_manager_t *asset_manager, render_state_t *render_state, asset_handle_t font_handle, vec2_t ending_pos, input_controller_t *controller);

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
