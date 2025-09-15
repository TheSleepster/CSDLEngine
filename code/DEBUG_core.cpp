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
    DEBUG_state_data_t *result = c_arena_bootstrap_allocate_struct(DEBUG_state_data_t, DEBUG_arena, MB(500));
    Assert(result != null);

    // TODO(Sleepster): Tune the cpu cycle -> ms conversion here. 

    result->is_collecting = true;
    return(result);
}

internal true_inline void
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
        event->cycle_counter = rdtsc();
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
        event->cycle_counter = rdtsc();
    }
}

internal u32 
DEBUG_register_performance_counter(char *filename, char *block_name, u32 line_number)
{
    for(u32 counter_index = 0;
        counter_index < DEBUG_global_state->next_debug_record_entry_index;
        ++counter_index)
    {
        DEBUG_record_t *old_record = DEBUG_global_state->record_array + counter_index;
        if(old_record->block_name != null && strcmp(old_record->block_name, block_name) == 0)
        {
            return(old_record->record_index);
        }
    }
    
    u32 result = AtomicIncrement(&DEBUG_global_state->next_debug_record_entry_index);

    DEBUG_record_t *record = DEBUG_global_state->record_array + result;
    record->filename       = filename;
    record->block_name     = block_name;
    record->line_number    = line_number;
    record->record_index   = result;

    return(result);
}

internal u32
DEBUG_get_thread_index(DEBUG_frame_data_t *current_frame, u64 thread_id)
{
    u32 result = (u32)-1;
    for(u32 thread_index = 0;
        thread_index < current_frame->thread_count;
        ++thread_index)
    {
        DEBUG_thread_data_t *thread = current_frame->thread_data + thread_index;
        if(thread->thread_id == thread_id)
        {
            result = thread_index;
            break;
        }
    }

    if(result == (u32)-1)
    {
        result = current_frame->thread_count++;
    }

    return(result);
}

internal DEBUG_thread_data_t*
DEBUG_get_thread(DEBUG_frame_data_t *current_frame, u64 thread_id)
{
    DEBUG_thread_data_t *result = 0;
    for(u32 thread_index = 0;
        thread_index < current_frame->thread_count;
        ++thread_index)
    {
        DEBUG_thread_data_t *found = current_frame->thread_data + thread_index;
        if(found->thread_id == thread_id)
        {
            result = found;
            break;
        }
    }

    if(result == null)
    {
        result = current_frame->thread_data + current_frame->thread_count++;
    }

    return(result);
}

internal void
DEBUG_handle_ui_input(input_controller_t *DEBUG_controller)
{
    // NOTE(Sleepster): Input stuff 
    {
        if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_COMMA))
        {
            if(DEBUG_global_state->current_frame_index > 0)
            {
                DEBUG_global_state->last_frame_index = DEBUG_global_state->current_frame_index - 1;
            }
            DEBUG_global_state->is_collecting = !DEBUG_global_state->is_collecting;
        }

        if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_PERIOD))
        {
            DEBUG_global_state->overlay_active = !DEBUG_global_state->overlay_active;
        }

        if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_LEFT) ||
           s_input_manager_is_keyboard_key_down(DEBUG_controller, SDL_SCANCODE_LEFT))
        {
            if(DEBUG_global_state->last_frame_index > 0)
            {
                DEBUG_global_state->last_frame_index -= 1;
            }
            else if(DEBUG_global_state->last_frame_index == 0)
            {
                DEBUG_global_state->last_frame_index = MAX_DEBUG_SNAPSHOTS - 1;
            }
        }

        if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_RIGHT) ||
           s_input_manager_is_keyboard_key_down(DEBUG_controller, SDL_SCANCODE_RIGHT))
        {
            if(DEBUG_global_state->last_frame_index < MAX_DEBUG_SNAPSHOTS - 1)
            {
                DEBUG_global_state->last_frame_index += 1;
            }
            else if(DEBUG_global_state->last_frame_index + 1 == DEBUG_global_state->current_frame_index)
            {
                DEBUG_global_state->last_frame_index = 0;
            }
            else if(DEBUG_global_state->last_frame_index + 1 == MAX_DEBUG_SNAPSHOTS)
            {
                DEBUG_global_state->last_frame_index = 0;
            }
        }
    
        if(s_input_manager_is_keyboard_key_pressed(DEBUG_controller, SDL_SCANCODE_SEMICOLON))
        {
            DEBUG_global_state->last_frame_index = DEBUG_global_state->current_frame_index - 1;
        }
    }
}

internal void
DEBUG_handle_events(input_controller_t *DEBUG_controller)
{
    DEBUG_handle_ui_input(DEBUG_controller);
    if(DEBUG_global_state->is_collecting)
    {
        DEBUG_event_t *event_array = DEBUG_global_state->event_array[DEBUG_global_state->event_array_index];
        for(u32 record_index = 0;
            record_index < DEBUG_global_state->next_debug_record_entry_index;
            ++record_index)
        {
            DEBUG_record_t *record = DEBUG_global_state->record_array + record_index;
            record->snapshots[DEBUG_global_state->current_frame_index].hit_count   = 0;
            record->snapshots[DEBUG_global_state->current_frame_index].cycle_count = 0;
        }
        DEBUG_frame_data_t *current_frame = DEBUG_global_state->frame_data + DEBUG_global_state->current_frame_index; 

        u32 event_array_index = !DEBUG_global_state->event_array_index;
        AtomicExchange32(&DEBUG_global_state->event_array_index, event_array_index);

        u32 event_array_count = AtomicExchange32(&DEBUG_global_state->next_debug_event_index, 0);
        for(u32 event_index = 0;
            event_index < event_array_count;
            ++event_index)
        {
            DEBUG_event_t  *event  = event_array + event_index;
            DEBUG_record_t *record = DEBUG_global_state->record_array + event->record_index;
        
            u32 thread_index = DEBUG_get_thread_index(current_frame, event->thread_id);
            DEBUG_thread_data_t *thread = current_frame->thread_data + DEBUG_get_thread_index(current_frame, event->thread_id);
            thread->thread_index = thread_index;
            switch(event->event_type)
            {
                case DEBUG_EVENT_TIMER_BEGIN:
                {
                    record->snapshots[DEBUG_global_state->current_frame_index].hit_count   += 1;
                    record->snapshots[DEBUG_global_state->current_frame_index].cycle_count -= event->cycle_counter;
                    if(!thread->is_valid)
                    {
                        thread->is_valid            = true;
                        thread->thread_id           = event->thread_id;
                        thread->first_open_block    = 0;
                    }

                    DEBUG_open_block_t *new_block = DEBUG_global_state->first_free_open_block;
                    if(new_block)
                    {
                        DEBUG_global_state->first_free_open_block = DEBUG_global_state->first_free_open_block->parent_block;
                    }
                    else
                    {
                        new_block = c_arena_push_struct(&DEBUG_global_state->DEBUG_arena, DEBUG_open_block_t);
                    }
                    Assert(new_block != null);

                    new_block->opening_event = event;
                    new_block->parent_block  = thread->first_open_block;
                    thread->first_open_block = new_block;

                    thread->first_open_block->parent_block = null;
                }break;
                case DEBUG_EVENT_TIMER_END:
                case DEBUG_EVENT_SECTION_MARK:
                {
                    record->snapshots[DEBUG_global_state->current_frame_index].cycle_count += event->cycle_counter;
                    if(thread->is_valid)
                    {
                        DEBUG_open_block_t *current_block = thread->first_open_block;
                        if((current_block->opening_event->thread_id    == event->thread_id) &&
                           (current_block->opening_event->record_index == event->record_index))
                        {
                            if(current_block->parent_block == null)
                            {
                                DEBUG_frame_section_t *section_data = current_frame->frame_sections + current_frame->frame_section_count; 
                                current_frame->frame_section_count = ((current_frame->frame_section_count + 1) % MAX_DEBUG_FRAME_SECTIONS);
                                
                                section_data->owner_thread_id = current_block->opening_event->thread_id;
                                section_data->min_clocks      = current_block->opening_event->cycle_counter - current_frame->begin_clock;
                                section_data->max_clocks      = event->cycle_counter - current_frame->begin_clock;
                                section_data->record          = DEBUG_global_state->record_array + event->record_index;
                            }
                            else
                            {
                                // TODO(Sleepster): Record child frames inbetween 
                            }
                        }
                        else
                        { 
                            // TODO(Sleepster): Record span that goes from beginning.
                        }
                    }
                }break;
                case DEBUG_EVENT_FRAME_END:
                {
                    u32 next_current_frame_index = (DEBUG_global_state->current_frame_index + 1) % MAX_DEBUG_SNAPSHOTS;
                    DEBUG_global_state->last_frame_index = AtomicExchange(&DEBUG_global_state->current_frame_index, next_current_frame_index);
                    current_frame->end_clock = event->cycle_counter;
                    float32 clock_range = current_frame->end_clock - current_frame->begin_clock;
                    if(clock_range > 0.0f)
                    {
                        float32 new_frame_bar_scale = 1.0f / clock_range;
                        if(DEBUG_global_state->frame_bar_scale < new_frame_bar_scale)
                        {
                            DEBUG_global_state->frame_bar_scale = new_frame_bar_scale;
                        }
                    }

                    current_frame = DEBUG_global_state->frame_data + DEBUG_global_state->current_frame_index; 
                    current_frame->begin_clock = event->cycle_counter;
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

internal vec2_t  
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
        DEBUG_snapshot_data_t *snapshot_data = record->snapshots  + DEBUG_global_state->last_frame_index;
        if(snapshot_data->hit_count > 0)
        {
            char buffer[4096] = {};

            snprintf(buffer,
                     sizeof(buffer),
                     "%-42s(%4d): %10llucy, %5lluh, %10llucy/h",
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
            DEBUG_global_state->last_frame_index);
    r_draw_string(asset_manager,
                  render_state,
                  STR(buffer),
                  font,
                  24,
                  starting_pos = vec2_add(starting_pos, vec2_create_float(0.0, -20.0)),
                  vec4_create(1.0f),
                  RQO_NONE);
    return(starting_pos);
}

internal void
DEBUG_render_section_graph(asset_manager_t *asset_manager, render_state_t *render_state, asset_handle_t font_handle, vec2_t ending_pos)
{
    DEBUG_frame_data *frame_data = DEBUG_global_state->frame_data + DEBUG_global_state->last_frame_index;
    if(frame_data)
    {
        vec4_t colors[] =
        {
            (vec4_t){1.0f, 1.0f, 1.0f, 1.0f},
            (vec4_t){1.0f, 0.0f, 0.0f, 1.0f},
            (vec4_t){0.0f, 1.0f, 0.0f, 1.0f},
            (vec4_t){0.0f, 0.0f, 1.0f, 1.0f},
            (vec4_t){1.0f, 1.0f, 0.0f, 1.0f},
            (vec4_t){0.0f, 1.0f, 1.0f, 1.0f},
            (vec4_t){1.0f, 0.0f, 1.0f, 1.0f},
        };
        
        vec2_t starting_graph_pos = ending_pos; 
        vec2_t bar_spacing        = vec2_create_float(10.0f, 0.0f);
        for(u32 section_index = 0;
            section_index < frame_data->frame_section_count;
            ++section_index)
        {
            DEBUG_frame_section_t *section = frame_data->frame_sections + section_index;
            vec4_t color = colors[section_index % ArrayCount(colors)];
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
        vec2_t ending_pos = DEBUG_display_record_data(asset_manager, render_state, font_handle, delta_time);
        DEBUG_render_section_graph(asset_manager, render_state, font_handle, ending_pos);
        r_end_renderpass(render_state);

        render_group_desc_t DEBUG_group_desc_transparent = DEBUG_group_desc;
        DEBUG_group_desc_transparent.render_layer = 1;
        r_begin_renderpass(render_state, &DEBUG_group_desc_transparent);
        r_draw_rect(render_state,
                    vec2_create_float(-960, -600),
                    vec2_create_float(1500, 2000),
                    vec4_create_float4(0.005f, 0.005f, 0.005f, 0.99f),
                    0,
                    RQO_NONE);
        r_end_renderpass(render_state);
    }
}

