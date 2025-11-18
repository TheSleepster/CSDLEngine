/* ========================================================================
   $File: r_render_group.cpp $
   $Date: November 15 2025 09:53 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#if 0

{
    r_set_active_render_material();
    r_set_render_camera();
    r_set_gpu_state();
    r_set_render_layer();
    r_set_texture();
    r_begin_renderpass();

    r_draw_things();

    r_end_renderpass();
}

#endif

#include "r_render_group.h"
#include "r_render_data.h"

internal void
r_render_group_init_geometry_buffer(render_state_t *render_state, geometry_buffer_t *buffer)
{
    buffer->quad_count  = 0;
    buffer->line_count  = 0;

    buffer->quad_vertex_count = 0;
    buffer->line_vertex_count = 0;
    buffer->next_buffer       = null;

    buffer->is_valid = true;
}

internal geometry_buffer_t*
r_render_group_get_buffer(render_state_t *render_state, render_group_t *render_group, u32 primitive_type)
{
    geometry_buffer_t *result      = null;
    geometry_buffer_t *last_buffer = null;

    for(geometry_buffer_t *buffer = &render_group->first_buffer;
        buffer;
        buffer = buffer->next_buffer)
    {
        Assert(buffer);
        if(!buffer->is_valid)
        {
            r_render_group_init_geometry_buffer(render_state, buffer);
        }

        switch(primitive_type)
        {
            case RGPT_Quads:
            {
                if(buffer->quad_count < MAX_QUADS)
                {
                    buffer->quad_vertex_buffer = c_arena_push_array(&render_state->persistant_arena, vertex_t,      MAX_VERTICES);
                    buffer->quad_buffer        = c_arena_push_array(&render_state->persistant_arena, render_quad_t, MAX_QUADS);

                    result = buffer;
                    goto r_geometry_buffer_found;
                }
            }break;
            case RGPT_Lines:
            {
                if(buffer->line_count < MAX_LINES)
                {
                    buffer->line_vertex_buffer = c_arena_push_array(&render_state->persistant_arena, vertex_t, MAX_VERTICES);
                    buffer->line_buffer        = c_arena_push_array(&render_state->persistant_arena, render_line_t,   MAX_LINES);

                    result = buffer;
                    goto r_geometry_buffer_found;
                }
            }break;
            default: {InvalidCodePath;}break;
        }

        last_buffer = buffer;
    }

    if(!result)
    {
        result = c_arena_push_struct(&render_state->persistant_arena, geometry_buffer_t);
        r_render_group_init_geometry_buffer(render_state, result);

        last_buffer->next_buffer = result;
    }
r_geometry_buffer_found:
    Assert(result);
    return(result);
}

internal void
r_render_group_process_quad_geometry(geometry_buffer_t *buffer)
{
    for(u32 quad_index = 0;
        quad_index < buffer->quad_count;
        ++quad_index)
    {
        render_quad_t *quad = buffer->quad_buffer + quad_index;

        vertex_t *vertex_buffer_ptr = buffer->quad_vertex_buffer + buffer->quad_vertex_count;
        vertex_t *top_left     = vertex_buffer_ptr + 0;
        vertex_t *top_right    = vertex_buffer_ptr + 1;
        vertex_t *bottom_right = vertex_buffer_ptr + 2;
        vertex_t *bottom_left  = vertex_buffer_ptr + 3;

        *top_left     = quad->top_left;
        *top_right    = quad->top_right;
        *bottom_left  = quad->bottom_left;
        *bottom_right = quad->bottom_right;

        buffer->quad_vertex_count += 4;
    }
}

internal void
r_render_group_process_line_geometry(geometry_buffer_t *buffer)
{
    for(u32 line_index = 0;
        line_index < buffer->line_count;
        ++line_index)
    {
        render_line_t *line = buffer->line_buffer + line_index;
        vertex_t *vertex_buffer_ptr = buffer->line_vertex_buffer + buffer->line_vertex_count;

        vertex_t *starting_point = vertex_buffer_ptr + 0;
        vertex_t *ending_point   = vertex_buffer_ptr + 1;

        *starting_point = line->start_point;
        *ending_point   = line->end_point;

        buffer->line_vertex_count += 2;
    }
}

internal void
r_render_group_handle_geometry_buffers(multithreading_work_queue_t *queue, render_group_t *render_group)
{
    DEBUG_TIMED_BLOCK();
    for(geometry_buffer_t *buffer  = &render_group->first_buffer;
        buffer;
        buffer = buffer->next_buffer)
    {
        if(buffer->quad_count > 0)
        {
            s_work_queue_add_entry(queue,
                                   (work_queue_callback_t*)r_render_group_process_quad_geometry,
                                   (void*)buffer);
        }

        if(buffer->line_count > 0)
        {
            s_work_queue_add_entry(queue,
                                   (work_queue_callback_t*)r_render_group_process_line_geometry,
                                   (void*)buffer);
        }
    }
}

internal render_group_t* 
r_render_group_create_new(render_state_t *render_state, hash_table_t *hash_table, u64 combo_id)
{
    render_group_t *result = c_arena_push_struct(&render_state->persistant_arena, render_group_t);
    result->group_ID    = combo_id;
    result->render_desc = (render_group_desc_t){
        .render_material = render_state->draw_frame.active_material,
        .render_phase    = render_state->draw_frame.active_render_phase,
        .camera          = render_state->draw_frame.active_camera,
        .pipeline_state  = render_state->pipeline_state,
    };

    u64 index = c_hash_create_key_index(hash_table, &combo_id, sizeof(u64));
    Assert(index > 0);

    result->phase_idx = index;

    hash_table_entry *entry = hash_table->entries + index;
    entry->value = result;
    entry->key   = STR((const char*)&combo_id);

    return(result);
}

internal inline void
r_renderpass_begin(render_state_t *render_state)
{
    draw_frame_data_t *draw_frame = &render_state->draw_frame;
    
    u32 material_id  = draw_frame->active_material.material_ID;
    u32 render_phase = draw_frame->active_render_phase;
    u64 combo_id     = ((u64)render_phase << 32) | (u64)material_id;

    render_phase_data_t *render_phase_data = null;
    if(render_phase == RGP_Preblit) render_phase_data  = &render_state->preblit_phase;
    if(render_phase == RGP_Postblit) render_phase_data = &render_state->postblit_phase;
    Assert(render_phase_data != null);

    render_group_container_t *container = null;
    if(render_state->pipeline_state.blending) container = &render_phase_data->transparent;
    else                                      container = &render_phase_data->opaque;
    Assert(container != null);

    render_group_t *render_group = (render_group_t*)c_hash_get_value(&container->group_hash, STR((const char*)&combo_id));
    if(!render_group)
    {
        render_group = r_render_group_create_new(render_state, &container->group_hash, combo_id);
    }

    Assert(render_group != null);
    draw_frame->active_render_group = render_group;

    u32 render_group_index = container->used_render_group_counter++;
    container->render_group_ids[render_group_index] = render_group->phase_idx;
}

internal inline void
r_renderpass_end(render_state_t *render_state)
{
    render_state->draw_frame.active_render_group = null;
}

internal void
r_renderpass_handle_data(render_state_t *render_state, asset_manager_t *asset_manager)
{
    DEBUG_TIMED_BLOCK();

    for(u32 group_index = 0;
        group_index < render_state->preblit_phase.opaque.used_render_group_counter;
        ++group_index)
    {
        u32 hash_index = render_state->preblit_phase.opaque.render_group_ids[group_index];

        render_group_t *render_group = (render_group_t*)render_state->preblit_phase.opaque.group_hash.entries[hash_index].value; 
        Assert(render_group);

        r_render_group_handle_geometry_buffers(&asset_manager->queue_manager->high_priority_queue, render_group);
    }

    for(u32 group_index = 0;
        group_index < render_state->preblit_phase.transparent.used_render_group_counter;
        ++group_index)
    {
        u32 hash_index = render_state->preblit_phase.transparent.render_group_ids[group_index];

        render_group_t *render_group = (render_group_t*)render_state->preblit_phase.transparent.group_hash.entries[hash_index].value; 
        Assert(render_group);

        r_render_group_handle_geometry_buffers(&asset_manager->queue_manager->high_priority_queue, render_group);
    }

    for(u32 group_index = 0;
        group_index < render_state->postblit_phase.opaque.used_render_group_counter;
        ++group_index)
    {
        u32 hash_index = render_state->postblit_phase.opaque.render_group_ids[group_index];

        render_group_t *render_group = (render_group_t*)render_state->postblit_phase.opaque.group_hash.entries[hash_index].value; 
        Assert(render_group);

        r_render_group_handle_geometry_buffers(&asset_manager->queue_manager->high_priority_queue, render_group);
    }

    for(u32 group_index = 0;
        group_index < render_state->postblit_phase.transparent.used_render_group_counter;
        ++group_index)
    {
        u32 hash_index = render_state->postblit_phase.transparent.render_group_ids[group_index];

        render_group_t *render_group = (render_group_t*)render_state->postblit_phase.transparent.group_hash.entries[hash_index].value; 
        Assert(render_group);

        r_render_group_handle_geometry_buffers(&asset_manager->queue_manager->high_priority_queue, render_group);
    }
}
