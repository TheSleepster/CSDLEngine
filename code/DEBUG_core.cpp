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

    result->is_collecting          = true;
    result->next_debug_event_index = 0;
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
    u32 last_last_frame_index = DEBUG_global_state->last_frame_index;
    u32 current_frame_index   = DEBUG_global_state->current_frame_index;
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

    if(last_last_frame_index != DEBUG_global_state->last_frame_index ||
       current_frame_index   != DEBUG_global_state->current_frame_index)
    {
        for(u32 thread_index = 0;
            thread_index < DEBUG_global_state->thread_count;
            ++thread_index)
        {
            DEBUG_thread_data_t *thread = DEBUG_global_state->threads + thread_index;
            memset((void*)thread->region_data, 0, sizeof(DEBUG_region_t) * thread->region_count);
            thread->region_count = 1;

            log_info("building call tree..\n");
            DEBUG_build_thread_call_tree(thread, thread_index);
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

internal u32
DEBUG_get_thread_index(u64 thread_id)
{
    Assert(DEBUG_global_state->thread_count <= MAX_DEBUG_THREADS);

    u32 result = (u32)-1;
    for(u32 thread_index = 0;
        thread_index < DEBUG_global_state->thread_count;
        ++thread_index)
    {
        DEBUG_thread_data_t *found = DEBUG_global_state->threads + thread_index;
        if(found->thread_id == thread_id)
        {
            result = thread_index;
            break;
        }
    }

    if(result == (u32)-1)
    {
        result = DEBUG_global_state->thread_count++;
    }

    return(result);
}

internal void
DEBUG_prune_thread_event_history(DEBUG_event_t *event)
{
    u32 oldest_frame_index = (DEBUG_global_state->current_frame_index + 1) % MAX_DEBUG_FRAME_HISTORY;
    u64 oldest_timestamp = DEBUG_global_state->frame_markers[oldest_frame_index];
    DEBUG_thread_data *thread = DEBUG_get_thread(event->thread_id);

    u32 first_invalid_event = thread->next_event_index;
    for(u32 event_index = thread->last_valid_event_index;
        event_index    != thread->next_event_index;
        event_index     = (event_index + 1) % MAX_DEBUG_EVENTS)
    {
        DEBUG_event_t *event = thread->events + event_index;
        if(event->cycle_counter <= oldest_timestamp)
        {
            first_invalid_event = event_index;
            break;
        }
    }

    if(first_invalid_event != thread->next_event_index)
    {
        thread->last_valid_event_index = first_invalid_event;
    }
}

internal void
DEBUG_prune_thread_stack_history(DEBUG_thread_data_t *thread)
{
    u32 oldest_frame_index = (DEBUG_global_state->current_frame_index + 1) % MAX_DEBUG_FRAME_HISTORY;
    u64 oldest_timestamp = DEBUG_global_state->frame_markers[oldest_frame_index];

    u32 first_invalid_scope = thread->built_scope_count; 
    for(u32 scope_index = thread->last_valid_scope_index;
        scope_index    != thread->built_scope_count;
        scope_index     = (scope_index + 1) % MAX_DEBUG_FRAME_SECTIONS)
    {
        DEBUG_scope_data_t *scope = thread->built_scope_stack + scope_index;
        if(scope->begin_clock <= oldest_timestamp)
        {
            first_invalid_scope = scope_index;
            break;
        }
    }

    if(first_invalid_scope != thread->built_scope_count)
    {
        thread->last_valid_scope_index = first_invalid_scope;
        for(u32 scope_index = thread->last_valid_scope_index;
            scope_index    != thread->built_scope_count;
            scope_index     = (scope_index + 1) % MAX_DEBUG_FRAME_SECTIONS)
        {
            DEBUG_scope_data_t *scope = thread->built_scope_stack + scope_index;
            if(scope->parent_scope_index != -1 && scope->parent_scope_index < (s32)thread->last_valid_scope_index)
            {
                scope->parent_scope_index = -1; 
            }
        }
    }
}

internal DEBUG_region_t*
DEBUG_find_or_create_region(DEBUG_thread_data_t *thread,
                            DEBUG_region_t      *parent, 
                            u32                  record_array_index)
{
    DEBUG_region_t *result = null;

    DEBUG_region_t *child   = parent->first_child;
    while(child)
    {
        if(child->record_index == record_array_index)
        {
            result = child;            
            break;
        }
        child   = child->next_sibling;
    }

    if(!result)
    {
        result = thread->region_data + thread->region_count++;
        result->record_index        = record_array_index;
        result->region_cycle_count  = 0;
        result->region_hit_count    = 0;
        result->region_thread_index = DEBUG_get_thread_index(thread->thread_id);
        result->first_child         = null;
        result->next_sibling        = parent->first_child;
        
        parent->first_child = result;
    }

    return(result);
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
DEBUG_build_thread_call_tree(DEBUG_thread_data_t *thread, u32 thread_index)
{
    DEBUG_region_t *thread_node = thread->region_data;
    thread_node->region_thread_index = thread_index;
    ZeroStruct(*thread_node);

    for(s32 scope_stack_index = (thread->built_scope_count - 1);
        scope_stack_index >= 0;
        --scope_stack_index)
    {
        DEBUG_scope_data_t *scope = thread->built_scope_stack + scope_stack_index;
        if(scope->frame_index == DEBUG_global_state->last_frame_index)
        {
            u64 scope_delta_cycles = scope->end_clock - scope->begin_clock;
            DEBUG_region_t *parent_region = thread_node;
            if(scope->parent_scope_index != -1)
            {
                DEBUG_scope_data_t *parent_scope = thread->built_scope_stack + scope->parent_scope_index;
                if((parent_scope->frame_index == DEBUG_global_state->last_frame_index) &&
                   (parent_scope->region_tree_node != null))
                {
                    parent_region = parent_scope->region_tree_node;
                }
            }
            Assert(parent_region);

            DEBUG_region_t *new_region = DEBUG_find_or_create_region(thread, parent_region, scope->record_array_index);
            new_region->region_cycle_count += scope_delta_cycles;
            new_region->parent_scope_index  = scope->parent_scope_index;
            new_region->region_hit_count   += 1;
            new_region->frame_index         = scope->frame_index;

            scope->region_tree_node = new_region;
            thread_node->region_cycle_count += scope_delta_cycles;
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
                    event->frame_index = DEBUG_global_state->current_frame_index;
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
            thread->top_most_stack_index = 0;
            thread->stack_depth          = -1;

            u32 thread_event_index = (thread->last_valid_event_index) % MAX_DEBUG_EVENTS;
            while(thread_event_index != thread->next_event_index)
            {
                DEBUG_event_t *event = thread->events + thread_event_index;
                switch(event->event_type)
                {
                    case DEBUG_EVENT_TIMER_BEGIN:
                    {
                        DEBUG_scope_data *new_scope   = thread->active_scope_stack + (thread->stack_depth + 1);
                        new_scope->begin_clock        = event->cycle_counter;
                        new_scope->record_array_index = event->record_index;
                        new_scope->end_clock          = 0;
                        new_scope->parent_scope_index = -1;
                        new_scope->frame_index        = event->frame_index;

                        thread->stack_depth += 1;
                    }break;
                    case DEBUG_EVENT_TIMER_END:
                    {
                        DEBUG_scope_data_t *our_open_block = thread->active_scope_stack + (thread->top_most_stack_index + thread->stack_depth);
                        our_open_block->end_clock = event->cycle_counter;
                        if(thread->built_scope_count == thread->last_valid_scope_index)
                        {
                            DEBUG_prune_thread_stack_history(thread);
                        }

                        u32 scope_index = thread->built_scope_count;
                        thread->built_scope_stack[scope_index] = *our_open_block;
                        thread->built_scope_stack[scope_index].region_tree_node = null;
                        thread->built_scope_count = (thread->built_scope_count + 1) % MAX_DEBUG_FRAME_SECTIONS;
                        if(thread->stack_depth > 0)
                        {
                            thread->built_scope_stack[scope_index].parent_scope_index = thread->built_scope_count; 
                        }

                        thread->stack_depth -= 1;
                    }break;
                }

                thread_event_index = (thread_event_index + 1) % MAX_DEBUG_EVENTS;
            }
        }

        // NOTE(Sleepster): Build the call tree for the thread 
        //u64 min_t = DEBUG_global_state->frame_markers[DEBUG_global_state->last_frame_index]; 
        //u64 max_t = DEBUG_global_state->frame_markers[DEBUG_global_state->current_frame_index]; 

        for(u32 thread_index = 0;
            thread_index < DEBUG_global_state->thread_count;
            ++thread_index)
        {
            DEBUG_thread_data_t *thread = DEBUG_global_state->threads + thread_index;
            memset((void*)thread->region_data, 0, sizeof(DEBUG_region_t) * thread->region_count);
            thread->region_count = 1;

            DEBUG_build_thread_call_tree(thread, thread_index);
        }
    }
}

internal u32 
DEBUG_get_graph_lane_depth(DEBUG_region_t *thread_root)
{
    u32 result = 0;
    
    DEBUG_flame_stack_t flame_stack[MAX_DEBUG_FRAME_SECTIONS];
    u32 stack_top = 0;

    flame_stack[0].region = thread_root;
    flame_stack[0].depth = 0;
    while(stack_top >= 0 &&
          stack_top  < MAX_DEBUG_FRAME_SECTIONS)
    {
        DEBUG_region_t *region = flame_stack[stack_top].region;
        u32 depth = flame_stack[stack_top--].depth;

        if(depth > result) result = depth;
        DEBUG_region_t *child = region->first_child;
        while(child)
        {
            flame_stack[++stack_top].region = child;
            flame_stack[stack_top].depth    = depth + 1;

            child = child->next_sibling;
        }
    }

    return(result);
}

internal void
DEBUG_render_section_graph(asset_manager_t    *asset_manager,
                           render_state_t     *render_state,
                           asset_handle_t     font_handle,
                           vec2_t             starting_pos,
                           input_controller_t *controller)
{
    // Render layers
    mat4_t font_projection_matrix = mat4_RHGL_ortho(-960, 960, -540, 540, -1, 1);
    mat4_t font_view_matrix = mat4_identity();
    render_group_desc_t background_layer = r_build_renderpass_desc(render_state,
                                                                   &render_state->font_shader,
                                                                   1,
                                                                   font_view_matrix,
                                                                   font_projection_matrix,
                                                                   RGE_None,
                                                                   RGP_PostBlitPass,
                                                                   RGPT_Quads);
    render_group_desc_t label_layer = background_layer;
    label_layer.render_layer = 4;

    const vec4_t colors[] =
        {
            {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f},
            {1.0f, 0.0f, 1.0f, 1.0f}, {0.4f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.1f, 0.1f, 1.0f}
        };
    const vec4_t background_color        = {0.03f, 0.03f, 0.03f, 0.99f};
    const vec4_t text_color              = {1.0f, 1.0f, 1.0f, 1.0f};
    const float32 lane_height_per_depth  = 20.0f;
    const float32 lane_width             = 1920.0f;
    const float32 min_bar_width_for_text = 50.0f;
    const float32 label_size             = 12.0f;
    const float32 tooltip_size           = 14.0f;
    const u32 max_depth_limit            = 32;

    u64 min_t = DEBUG_global_state->frame_markers[DEBUG_global_state->last_frame_index];
    u64 max_t = DEBUG_global_state->frame_markers[DEBUG_global_state->current_frame_index];
    u64 frame_cycles = max_t - min_t;
    float32 cycle_to_pixel_scale = (frame_cycles > 0) ? lane_width / (float32)frame_cycles : 0.0f;

    vec2_t current_pos = vec2_add(starting_pos, {0.0f, 60.0f});
    for(u32 thread_index = 0; thread_index < DEBUG_global_state->thread_count; ++thread_index)
    {
        DEBUG_thread_data_t *thread = DEBUG_global_state->threads + thread_index;
        DEBUG_region_t *thread_root = thread->region_data;

        u32 max_depth       = DEBUG_get_graph_lane_depth(thread_root);
        float32 lane_height = (float32)max_depth * lane_height_per_depth; 

        // NOTE(Sleepster): Thread label
        r_begin_renderpass(render_state, &background_layer);

        char label[256];
        snprintf(label, sizeof(label), "Thread %u (ID: %llu, %llu cy)", thread_index, thread->thread_id, thread_root->region_cycle_count);
        r_draw_string(asset_manager, render_state, STR(label), font_handle, 16, current_pos, text_color, RQO_NONE);

        r_end_renderpass(render_state);

        current_pos.y += 20.0f;

        DEBUG_render_stack_data_t stack[1024];
        u32 stack_top = -1;
        stack[++stack_top].region = thread_root;
        stack[stack_top].depth = 0;
        stack[stack_top].start_x = 0.0f;

        while(stack_top >= 0 && stack_top < 1024)
        {
            DEBUG_region_t *region = stack[stack_top].region;
            u32 depth = stack[stack_top].depth;
            float32 start_x = stack[stack_top--].start_x;
            if(!region || depth >= max_depth_limit) continue;

            if(depth > 0)
            {
                u64 delta = region->region_cycle_count;
                float32 x = current_pos.x + start_x;
                float32 y = current_pos.y + (float32)(depth - 1) * lane_height_per_depth; 
                float32 width = (float32)delta * cycle_to_pixel_scale;
                float32 height = lane_height_per_depth;

                // NOTE(Sleepster): Draw bar
                r_begin_renderpass(render_state, &background_layer);
                u32 color_idx = (region->record_index * 31) % ArrayCount(colors);
                r_draw_rect(render_state,
                            vec2_create_float(x, y),
                            vec2_create_float(width, height),
                            colors[color_idx],
                            0,
                            RQO_NONE);

                // NOTE(Sleepster): Label if wide enough
                if(width > min_bar_width_for_text)
                {
                    if(region->record_index < DEBUG_global_state->next_debug_record_entry_index)
                    {
                        DEBUG_record_t *record = DEBUG_global_state->record_array + region->record_index;
                        char label[4096];
                        snprintf(label,
                                 sizeof(label),
                                 "%s (%u hits)",
                                 record->block_name,
                                 region->region_hit_count);
                        r_draw_string(asset_manager,
                                      render_state,
                                      STR(label),
                                      font_handle,
                                      label_size,
                                      vec2_create_float(x + 2.0f, y + 2.0f),
                                      text_color,
                                      RQO_NONE);
                    }
                }
                r_end_renderpass(render_state);

                // NOTE(Sleepster): Tooltip
                vec2_t mouse = s_input_manager_transform_mouse_data(controller, font_view_matrix, font_projection_matrix);
                rectangle2_t bar_rect = rect_create(vec2_create_float(x, y), vec2_add(vec2_create_float(x, y), vec2_create_float(width, height)));
                if(rect_vec2_test(bar_rect, mouse))
                {
                    r_begin_renderpass(render_state, &background_layer);
                    DEBUG_record_t *record = DEBUG_global_state->record_array + region->record_index;
                    char buffer[4096];
                    snprintf(buffer,
                             sizeof(buffer),
                             "%s: [%s, %d]\nTotal: %llu cy\nHits: %u\nThread: %llu",
                             record->block_name,
                             record->filename,
                             record->line_number,
                             region->region_cycle_count,
                             region->region_hit_count,
                             (u64)region->region_thread_index);
                    vec2_t tooltip_pos = vec2_add(mouse, vec2_create_float(10.0f, -20.0f));
                    r_draw_rect(render_state, vec2_subtract(tooltip_pos, {0.0f, 50.0f}), vec2_create_float(500.0f, 100.0f), background_color, 0, RQO_NONE);
                    r_end_renderpass(render_state);

                    r_begin_renderpass(render_state, &label_layer);
                    r_draw_string(asset_manager,
                                  render_state,
                                  STR(buffer),
                                  font_handle,
                                  tooltip_size,
                                  vec2_add(tooltip_pos, vec2_create_float(5.0f, 5.0f)),
                                  text_color,
                                  RQO_NONE);
                    r_end_renderpass(render_state);
                }
            }

            u32 child_count = 0;
            DEBUG_region_t *children[1024];
            DEBUG_region_t *child = region->first_child;
            while(child)
            {
                children[child_count++] = child;
                child = child->next_sibling;
            }

            for(s32 child_index = child_count - 1;
                child_index >= 0;
                --child_index)
            {
                stack[++stack_top].region = children[child_index];
                stack[stack_top].depth = depth + 1;
                stack[stack_top].start_x = start_x; 

                start_x += (float32)children[child_index]->region_cycle_count * cycle_to_pixel_scale;
            }
        }

        current_pos.y += lane_height + 10.0f;
        thread->region_count = 1;
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
                     (unsigned long long)snapshot_data->cycle_count,
                     (unsigned long long)snapshot_data->hit_count,
                     (unsigned long long)(snapshot_data->cycle_count / snapshot_data->hit_count));
            
            #if 0
            r_draw_string(asset_manager,
                          render_state,
                          STR(buffer),
                          font,
                          24,
                          starting_pos,
                          vec4_create(1.0f),
                          RQO_NONE);
            #endif

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
        vec2_t ending_pos = DEBUG_display_record_data(asset_manager, render_state, font_handle, delta_time);
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

        DEBUG_render_section_graph(asset_manager, render_state, font_handle, ending_pos, controller);

        r_set_active_blending_state(render_state, false);
        r_set_active_depth_state(render_state, true, true);
    }
}
