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
    DEBUG_state_data_t *result = c_arena_bootstrap_allocate_struct(DEBUG_state_data_t, DEBUG_arena, GB(4));
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
        event->frame_index   = DEBUG_global_state->current_frame_index;
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

internal DEBUG_thread_data_t*
DEBUG_get_thread(u64 thread_id)
{
    Assert(DEBUG_global_state->thread_count <= MAX_DEBUG_THREADS);

    DEBUG_thread_data_t *result = null;
    for(u32 thread_index = 0;
        thread_index < DEBUG_global_state->thread_count;
        ++thread_index)
    {
        DEBUG_thread_data_t *found = DEBUG_global_state->threads + thread_index;
        if(found->thread_id == thread_id)
        {
            result = found;
            break;
        }
    }

    if(!result)
    {
        result = DEBUG_global_state->threads + DEBUG_global_state->thread_count++;
        result->thread_id              = thread_id;
        result->last_valid_event_index = MAX_DEBUG_EVENTS;
    }

    return(result);
}

// grab the timestamp for the oldest frame.
// find the first scope that is older or as old as the oldest timestamp
// find the value that is greater than the oldest timestamp.
// 

internal void
DEBUG_prune_thread_event_history(DEBUG_event_t *event)
{
    u32 oldest_frame_index = (DEBUG_global_state->current_frame_index + 1) % MAX_DEBUG_FRAME_HISTORY;
    u64 oldest_timestamp   = DEBUG_global_state->frame_markers[oldest_frame_index];

    DEBUG_thread_data *thread = DEBUG_get_thread(event->thread_id);

    // NOTE(Sleepster): Prune events 
    {
        u32 first_invalid_event = 0;
        for(u32 event_index = 0;
            event_index < thread->next_event_index;
            ++event_index)
        {
            DEBUG_event_t *event = thread->events + event_index;
            if(event->cycle_counter <= oldest_timestamp)
            {
                first_invalid_event = event_index;
                break;
            }
        } 
        Assert(first_invalid_event != thread->next_event_index);

        u32 last_valid_index = thread->last_valid_event_index;
        for(u32 event_index = first_invalid_event;
            event_index < last_valid_index;
            ++event_index)
        {
            DEBUG_event_t *event = thread->events + event_index;
            if(event->cycle_counter >= oldest_timestamp)
            {
                break;
            }

            thread->last_valid_event_index = (thread->last_valid_event_index + 1) % MAX_DEBUG_EVENTS;
        }
    }
}

internal void
DEBUG_prune_thread_stack_history(DEBUG_thread_data_t *thread)
{
    u32 oldest_frame_index = (DEBUG_global_state->current_frame_index + 1) % MAX_DEBUG_FRAME_HISTORY;
    u64 oldest_timestamp   = DEBUG_global_state->frame_markers[oldest_frame_index];

    u32 first_invalid_scope = 0;
    for(u32 scope_index = 0;
        scope_index < thread->built_scope_count;
        ++scope_index)
    {
        DEBUG_scope_data_t *scope = thread->built_scope_stack + scope_index;
        if(scope->begin_clock <= oldest_timestamp)
        {
            first_invalid_scope = scope_index;
            break;
        }
    }

    u32 last_valid_index = thread->last_valid_scope_index;
    for(u32 scope_index = first_invalid_scope;
        scope_index < last_valid_index;
        ++scope_index)
    {
        DEBUG_scope_data_t *scope = thread->built_scope_stack + scope_index;
        if(scope->begin_clock >= oldest_timestamp)
        {
            break;
        }

        thread->last_valid_scope_index = (thread->last_valid_scope_index + 1) % MAX_DEBUG_FRAME_SECTIONS;
    }
}

internal void 
DEBUG_append_thread_event(DEBUG_event_t *event)
{
    DEBUG_thread_data *thread = DEBUG_get_thread(event->thread_id);

    if(thread->next_event_index == thread->last_valid_event_index)
    {
        DEBUG_prune_thread_event_history(event);
    }

    thread->events[thread->next_event_index] = *event;
    thread->next_event_index = (thread->next_event_index + 1) % MAX_DEBUG_EVENTS;
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

        u32 event_array_index = !DEBUG_global_state->event_array_index;
        AtomicExchange32(&DEBUG_global_state->event_array_index, event_array_index);

        u32 event_array_count = AtomicExchange32(&DEBUG_global_state->next_debug_event_index, 0);
        for(u32 event_index = 0;
            event_index < event_array_count;
            ++event_index)
        {
            DEBUG_event_t  *event       = event_array + event_index;
            DEBUG_record_t *record      = DEBUG_global_state->record_array + event->record_index;
            switch(event->event_type)
            {
                case DEBUG_EVENT_TIMER_BEGIN:
                {
                    record->snapshots[DEBUG_global_state->current_frame_index].hit_count   += 1;
                    record->snapshots[DEBUG_global_state->current_frame_index].cycle_count -= event->cycle_counter;
                    DEBUG_append_thread_event(event);
                }break;
                case DEBUG_EVENT_TIMER_END:
                {
                    record->snapshots[DEBUG_global_state->current_frame_index].cycle_count += event->cycle_counter;
                    DEBUG_append_thread_event(event);
                }break;
                case DEBUG_EVENT_FRAME_END:
                {
                    u32 next_current_frame_index = (DEBUG_global_state->current_frame_index + 1) % MAX_DEBUG_SNAPSHOTS;

                    DEBUG_global_state->frame_markers[DEBUG_global_state->last_frame_index] = event->cycle_counter;
                    DEBUG_global_state->last_frame_index   = AtomicExchange(&DEBUG_global_state->current_frame_index, next_current_frame_index);
                }break;
                case DEBUG_EVENT_RELOAD_DLL:
                {
                    DEBUG_global_state->should_reload_dll = true;
                }break;
                default: {}break;
            }
        }

        // NOTE(Sleepster): build thread scopes 
        for(u32 thread_index = 0;
            thread_index < DEBUG_global_state->thread_count;
            ++thread_index)
        {
            DEBUG_thread_data_t *thread = DEBUG_global_state->threads + thread_index;

            u32 thread_event_index = thread->last_valid_event_index;
            while(thread_event_index != thread->next_event_index)
            {
                DEBUG_event_t *event = thread->events + thread_event_index;
                switch(event->event_type)
                {
                    case DEBUG_EVENT_TIMER_BEGIN:
                    {
                        DEBUG_scope_data *new_scope   = thread->active_scope_stack + thread->top_most_stack_index;
                        s32 new_scope_parent_index    = -1;
                        if(thread->top_most_stack_index > 0)
                        {
                            new_scope_parent_index = (thread->top_most_stack_index - 1) % MAX_DEBUG_FRAME_SECTIONS;
                        }

                        new_scope->begin_clock        = event->cycle_counter;
                        new_scope->record_array_index = event->record_index;
                        new_scope->end_clock          = 0;
                        new_scope->parent_scope       = new_scope_parent_index;

                        thread->top_most_stack_index  = (thread->top_most_stack_index + 1) % MAX_DEBUG_FRAME_SECTIONS;
                    }break;
                    case DEBUG_EVENT_TIMER_END:
                    {
                        u32 scope_index = (thread->top_most_stack_index + MAX_DEBUG_FRAME_SECTIONS - 1) % MAX_DEBUG_FRAME_SECTIONS;
                        thread->top_most_stack_index = scope_index;

                        DEBUG_scope_data_t *scope = thread->active_scope_stack + scope_index;
                        scope->end_clock = event->cycle_counter;
                        if(thread->built_scope_count == thread->last_valid_scope_index)
                        {
                            DEBUG_prune_thread_stack_history(thread);
                        }

                        thread->built_scope_stack[thread->built_scope_count] = *scope;
                        thread->built_scope_count = (thread->built_scope_count + 1) % MAX_DEBUG_FRAME_SECTIONS;
                    } break;
                }

                thread_event_index = (thread_event_index + 1) % MAX_DEBUG_EVENTS;
            }
        }

    }
}

#if 0
internal void
DEBUG_render_section_graph(asset_manager_t    *asset_manager, 
                           render_state_t     *render_state, 
                           asset_handle_t      font_handle, 
                           vec2_t              ending_pos,
                           input_controller_t *controller)
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
            (vec4_t){0.4f, 0.0f, 1.0f, 1.0f},
            (vec4_t){1.0f, 0.1f, 0.1f, 1.0f},
        };
        
        vec2_t starting_graph_pos = vec2_subtract(ending_pos, vec2_create_float(0, 100)); 
        vec2_t bar_spacing        = vec2_create_float(30.0f, 0.0f);

        float32 lane_width   = 20.0f;
        float32 chart_height = 300.0f; 
        float32 chart_min_y  = starting_graph_pos.y - chart_height;
        
        float32 bar_scale = DEBUG_global_state->frame_bar_scale;
        for(u32 section_index = 0;
            section_index < frame_data->section_count;
            ++section_index)
        {
            DEBUG_frame_section_t *section = frame_data->sections + section_index;
            if(section->record)
            {
                vec4_t color = colors[section->record->record_index % ArrayCount(colors)];

                float32 stackx = starting_graph_pos.x + bar_spacing.x * (float32)section_index;
                float32 stacky = chart_min_y;
                float32 bar_min_t = stacky + (section->min_clocks * bar_scale);
                float32 bar_max_t = stacky + (section->max_clocks * bar_scale);

                vec2_t rect_pos  = vec2_create_float(stackx, stacky);
                vec2_t rect_size = vec2_create_float(lane_width, chart_height * fabs(bar_max_t - bar_min_t));

                r_draw_rect(render_state, rect_pos, rect_size, color, 0, RQO_NONE);
                rectangle2_t cursor_box = rect_create(rect_pos, vec2_add(rect_pos, rect_size));

                vec2_t mouse_cursor_pos = s_input_manager_transform_mouse_data(controller,
                                                                               render_state->draw_frame.active_render_group->render_desc.view_matrix, 
                                                                               render_state->draw_frame.active_render_group->render_desc.projection_matrix);
                if(rect_vec2_test(cursor_box, mouse_cursor_pos))
                {
                    char buffer[4096] = {};
                    sprintf(buffer,
                            "%s: [%s, %d]\nClocks: '%llu'cy. Frame Index: '%d'\nSection Index: '%d'\nThread ID: '%llu'",
                            section->record->block_name,
                            section->record->filename,
                            section->record->line_number,
                            (unsigned long long)(section->max_clocks - section->min_clocks),
                            section->frame_index,
                            section_index,
                            (unsigned long long)section->owner_thread_id);
                    r_draw_string(asset_manager,
                                  render_state,
                                  STR(buffer),
                                  font_handle,
                                  24,
                                  vec2_add(rect_pos, vec2_create_float(40.0, 100.0f)),
                                  vec4_create(1.0f),
                                  RQO_NONE);
                }
            } 
        }
    }
}
#endif

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
                     (unsigned long long)snapshot_data->cycle_count,
                     (unsigned long long)snapshot_data->hit_count,
                     (unsigned long long)(snapshot_data->cycle_count / snapshot_data->hit_count));
            
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
DEBUG_render_group_to_output(input_controller_t *controller, asset_manager_t *asset_manager, render_state_t *render_state, float32 delta_time)
{
    if(DEBUG_global_state->overlay_active)
    {
        asset_handle_t font_handle =  s_asset_font_get(asset_manager, STR("LiberationMono_Regular"));
    
        mat4_t font_projection_matrix = mat4_RHGL_ortho(-960, 960, -540, 540, -1, 1);
        mat4_t font_view_matrix       = mat4_identity();
        render_group_desc_t DEBUG_group_desc = r_build_renderpass_desc(render_state,
                                                                       &render_state->font_shader,
                                                                       0,
                                                                       font_view_matrix,
                                                                       font_projection_matrix,
                                                                       RGE_None,
                                                                       RGP_PostBlitPass,
                                                                       RGPT_Quads);
        r_begin_renderpass(render_state, &DEBUG_group_desc);
        DEBUG_display_record_data(asset_manager, render_state, font_handle, delta_time);
        //DEBUG_render_section_graph(asset_manager, render_state, font_handle, ending_pos, controller);
        r_end_renderpass(render_state);

        r_set_active_blending_state(render_state, true);
        r_set_active_depth_state(render_state, true, false);
        r_set_active_blend_mode(render_state, RGBM_One, RGBM_OneMinusSrcAlpha, RGBM_One, RGBM_OneMinusSrcAlpha);
        render_group_desc_t DEBUG_group_desc_transparent = r_build_renderpass_desc(render_state,
                                                                                   &render_state->font_shader,
                                                                                   1,
                                                                                   font_view_matrix,
                                                                                   font_projection_matrix,
                                                                                   RGE_None,
                                                                                   RGP_PostBlitPass,
                                                                                   RGPT_Quads);
        r_begin_renderpass(render_state, &DEBUG_group_desc_transparent);
        r_draw_rect(render_state,
                    vec2_create_float(-960, -600),
                    vec2_create_float(1500, 2000),
                    vec4_create_float4(0.003f, 0.003f, 0.003f, 0.99f),
                    0,
                    RQO_NONE);
        r_end_renderpass(render_state);

        r_set_active_blending_state(render_state, false);
        r_set_active_depth_state(render_state, true, true);
    }
}

