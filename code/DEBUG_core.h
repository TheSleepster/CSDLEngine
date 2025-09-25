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
#define MAX_DEBUG_SNAPSHOTS      (MAX_DEBUG_FRAME_HISTORY)
#define MAX_DEBUG_FRAME_SECTIONS (8192 * 8)
#define MAX_DEBUG_COUNTERS       (1024)
#define MAX_DEBUG_EVENTS         (8192 * 4)
#define MAX_DEBUG_THREADS        (32)

#define DEBUG_TIMED_BLOCK()                                             \
    local_persist u32 DEBUG_record_index = (u32)-1;                     \
    if(DEBUG_record_index == (u32)-1)                                   \
        DEBUG_record_index = DEBUG_register_performance_counter((char *)__FILE__, (char *)__FUNCTION__, __LINE__); \
    new_timed_block_t block_timer_##__LINE__(DEBUG_record_index);        

typedef struct render_group render_group_t;
typedef struct asset_manager asset_manager_t;
typedef struct render_state render_state_t;
typedef struct asset_handle asset_handle_t;
typedef struct input_controller input_controller_t;

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
    u32   record_index;
    u32   core_index;
    u64   thread_id;
    u64   cycle_counter;
}DEBUG_event_t;

typedef struct DEBUG_region
{
    u32             record_index;
    u64             region_cycle_count;
    u32             region_hit_count;
    u32             region_thread_index;

    s32             parent_scope_index; // for debugging
    s32             frame_index;

    DEBUG_region   *first_child;
    DEBUG_region   *next_sibling;
}DEBUG_region_t;

typedef struct DEBUG_scope_data
{
    u64             begin_clock;
    u64             end_clock;
    u32             record_array_index;
    s32             parent_scope_index;
    u32             frame_index; // for debugging

    DEBUG_region_t *region_tree_node;
}DEBUG_scope_data_t;

typedef struct DEBUG_thread_data
{
    u32                next_event_index;
    u32                last_valid_event_index;
    u64                thread_id;
    DEBUG_event_t      events[MAX_DEBUG_EVENTS];

    s32                top_most_stack_index;
    s32                stack_depth;
    DEBUG_scope_data_t active_scope_stack[MAX_DEBUG_FRAME_SECTIONS];

    u32                built_scope_count;
    u32                last_valid_scope_index;
    DEBUG_scope_data_t built_scope_stack[MAX_DEBUG_FRAME_SECTIONS];

    u32                region_count;
    DEBUG_region_t     region_data[MAX_DEBUG_FRAME_SECTIONS];
}DEBUG_thread_data_t;

typedef struct DEBUG_state_data
{
    memory_arena_t      DEBUG_arena;
    u64                 cpu_freq;
    
    volatile u32        next_debug_record_entry_index;
    volatile u32        next_debug_event_index;
    volatile u32        event_array_index;
    volatile u32        current_frame_index;
    volatile u32        last_frame_index;

    bool8               should_reload_dll;
    bool8               is_collecting;
    bool8               overlay_active;

    u32                 thread_count;
    DEBUG_thread_data_t threads[MAX_DEBUG_THREADS]; 

    u64                 frame_markers[MAX_DEBUG_FRAME_HISTORY];

    DEBUG_record_t      record_array[MAX_DEBUG_COUNTERS];
    DEBUG_event_t       event_array[2][MAX_DEBUG_EVENTS];
}DEBUG_state_data_t;

struct DEBUG_render_stack_data_t
{
    DEBUG_region_t *region;
    u32             depth;
    float32         start_x;
};

struct DEBUG_flame_stack_t
{
    DEBUG_region_t *region;
    u32             depth;
};

internal void                DEBUG_handle_ui_input(input_controller_t *DEBUG_controller);
internal DEBUG_state_data_t *DEBUG_create_debug_state();
internal true_inline void    DEBUG_record_event(u32 record_index, u8 type);
internal true_inline void    DEBUG_record_allocation_event(u8 type, u64 allocation_size);
internal true_inline void    DEBUG_set_event_marker(u8 type);
internal u32                 DEBUG_register_performance_counter(char *filename, char *block_name, u32 line_number);
internal void                DEBUG_handle_events();
internal vec2_t              DEBUG_display_record_data(asset_manager_t *asset_manager, render_state_t *render_state, asset_handle_t font, float32 delta_time);
internal void                DEBUG_render_section_graph(asset_manager_t *asset_manager, render_state_t *render_state, asset_handle_t font_handle, vec2_t starting_pos, input_controller_t *controller);
internal void                DEBUG_build_thread_call_tree(DEBUG_thread_data_t *thread, u32 thread_index);

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
