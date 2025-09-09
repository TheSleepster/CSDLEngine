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
#include "r_render_API.h"

internal DEBUG_state_data_t*
DEBUG_create_debug_state()
{
    DEBUG_state_data_t *result = c_arena_bootstrap_allocate_struct(DEBUG_state_data_t, DEBUG_arena, MB(100));
    Assert(result != null);

    // TODO(Sleepster): Tune the cpu cycle -> ms conversion here. 

    return(result);
}

internal inline void
DEBUG_record_event(u32 record_index, u8 type)
{
    if(DEBUG_global_state->is_collecting)
    {
        Assert(DEBUG_global_state->next_debug_event_index < MAX_DEBUG_EVENTS);
    
        u32 event_index = AtomicIncrement(&DEBUG_global_state->next_debug_event_index);
        Assert(event_index < MAX_DEBUG_EVENTS);

        DEBUG_event_t *event = DEBUG_global_state->event_array[DEBUG_global_state->event_array_index] + event_index;
        event->event_type    = type;
        event->record_index  = record_index;
        event->thread_id     = GetThreadID();

        u32 processor_id = 0;
        event->cycle_counter = rdtscp(&processor_id);
    }
}

internal inline void
DEBUG_set_event_marker(u8 type)
{
    if(DEBUG_global_state->is_collecting)
    {
        u32 event_index = AtomicIncrement(&DEBUG_global_state->next_debug_event_index);
        DEBUG_event_t *event = DEBUG_global_state->event_array[DEBUG_global_state->event_array_index] + event_index;
        event->event_type    = type;
    }
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
DEBUG_handle_events(input_controller_t *DEBUG_controller)
{
    if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_COMMA))
    {
        if(DEBUG_global_state->snapshot_index > 0)
        {
            DEBUG_global_state->last_snapshot_index = DEBUG_global_state->snapshot_index - 1;
        }
        DEBUG_global_state->is_collecting       = !DEBUG_global_state->is_collecting;
    }

    if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_PERIOD))
    {
        DEBUG_global_state->overlay_active = !DEBUG_global_state->overlay_active;
    }

    if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_LEFT) ||
       s_input_manager_is_keyboard_key_down(DEBUG_controller, SDL_SCANCODE_LEFT))
    {
        if(DEBUG_global_state->last_snapshot_index > 0)
        {
            DEBUG_global_state->last_snapshot_index -= 1;
        }
        else if(DEBUG_global_state->last_snapshot_index == 0)
        {
            DEBUG_global_state->last_snapshot_index = MAX_DEBUG_SNAPSHOTS - 1;
        }
    }

    if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_RIGHT) ||
       s_input_manager_is_keyboard_key_down(DEBUG_controller, SDL_SCANCODE_RIGHT))
    {
        if(DEBUG_global_state->last_snapshot_index < MAX_DEBUG_SNAPSHOTS - 1)
        {
            DEBUG_global_state->last_snapshot_index += 1;
        }
        else if(DEBUG_global_state->last_snapshot_index + 1 == DEBUG_global_state->snapshot_index)
        {
            DEBUG_global_state->last_snapshot_index = 0;
        }
        else if(DEBUG_global_state->last_snapshot_index + 1 == MAX_DEBUG_SNAPSHOTS)
        {
            DEBUG_global_state->last_snapshot_index = 0;
        }
    }

    if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_SEMICOLON))
    {
        DEBUG_global_state->last_snapshot_index = DEBUG_global_state->snapshot_index - 1;
    }

    if(DEBUG_global_state->is_collecting)
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
}

internal void
DEBUG_render_group_to_output(asset_manager_t *asset_manager, render_state_t *render_state, float32 delta_time)
{
    if(DEBUG_global_state->overlay_active)
    {
        asset_handle_t font_handle =  s_asset_font_get(asset_manager, STR("LiberationMono_Regular"));
    
        mat4_t font_projection_matrix = mat4_RHGL_ortho(-960, 960, -540, 540, -1, 1);
        mat4_t font_view_matrix       = mat4_identity();
        render_group_desc_t DEBUG_group_desc = r_build_renderpass_desc(&render_state->font_shader,
                                                                       0,
                                                                       font_view_matrix,
                                                                       font_projection_matrix,
                                                                       RGE_None,
                                                                       RGP_PostBlitPass);
        r_begin_renderpass(render_state, &DEBUG_group_desc);
        DEBUG_display_record_data(asset_manager, render_state, font_handle, delta_time);
        r_end_renderpass(render_state);

        render_group_desc_t DEBUG_group_desc_transparent = DEBUG_group_desc;
        DEBUG_group_desc_transparent.render_layer = 1;
        r_begin_renderpass(render_state, &DEBUG_group_desc_transparent);
        r_draw_rect(render_state, vec2_create_float(-960, -600), vec2_create_float(1250, 2000), vec4_create_float4(0.005f, 0.005f, 0.005f, 0.99f), 0, RQO_NONE);
        r_end_renderpass(render_state);
    }
}

internal void
DEBUG_display_record_data(asset_manager_t *asset_manager,
                          render_state_t  *render_state,
                          asset_handle_t   font,
                          float32          delta_time)
{
    vec2_t starting_pos  = vec2_create_float(-960, 500);
    for(u32 record_index = 0;
        record_index < DEBUG_global_state->next_debug_record_entry_index;
        ++record_index)
    {
        DEBUG_record_t *record = DEBUG_global_state->record_array + record_index;
        DEBUG_snapshot_data_t *snapshot_data = record->snapshots  + DEBUG_global_state->last_snapshot_index;
        if(snapshot_data->hit_count > 0)
        {
            char buffer[4096] = {};

            _snprintf_s(buffer,
                        sizeof(buffer),
                        "%-36s(%4d): %10llu cy, %8llu h, %10llu cy/h",
                        record->block_name,
                        record->line_number,
                        snapshot_data->cycle_count,
                        snapshot_data->hit_count,
                        snapshot_data->cycle_count / snapshot_data->hit_count);
            
            r_draw_string(asset_manager,
                          render_state,
                          STR(buffer),
                          font,
                          24,
                          starting_pos,
                          vec4_create(1.0f),
                          RQO_NONE);

            starting_pos = vec2_add(starting_pos, vec2_create_float(0, -32));
        }
    }
    char buffer[4096] = {};
    sprintf(buffer,
            "Total frame time is: %.04fms\nFrame Index is: %d\n",
            delta_time,
            DEBUG_global_state->last_snapshot_index);
    r_draw_string(asset_manager,
                  render_state,
                  STR(buffer),
                  font,
                  24,
                  vec2_add(starting_pos, vec2_create_float(0.0, -20.0)),
                  vec4_create(1.0f),
                  RQO_NONE);
}


