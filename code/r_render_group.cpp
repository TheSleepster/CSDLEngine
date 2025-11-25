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
    buffer->render_camera     = render_state->draw_frame.active_camera;

    buffer->is_valid = true;
}

// TODO(Sleepster): Add a "active_buffer" to the render_group_t structure
// so that we don't have to query this EVERY SINGLE TIME. We can just set it once
// and check the primitive count too see if the active_buffer needs to be changed.
internal geometry_buffer_t*
r_render_group_get_buffer(render_state_t *render_state, render_group_t *render_group, u32 primitive_type)
{
    DEBUG_TIMED_BLOCK();

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

        if(buffer->render_camera == render_state->draw_frame.active_camera)
        {
            switch(primitive_type)
            {
                case RGPT_Quads:
                {
                    if(buffer->quad_count < MAX_QUADS)
                    {
                        if(!buffer->quad_vertex_buffer && !buffer->quad_buffer)
                        {
                            buffer->quad_vertex_buffer = c_arena_push_array(&render_state->persistant_arena, vertex_t,      MAX_VERTICES);
                            buffer->quad_buffer        = c_arena_push_array(&render_state->persistant_arena, render_quad_t, MAX_QUADS);
                        }
                        Assert(buffer->quad_vertex_buffer != null);
                        Assert(buffer->quad_buffer != null);

                        result = buffer;
                        goto r_geometry_buffer_found;
                    }
                }break;
                case RGPT_Lines:
                {
                    if(buffer->line_count < MAX_LINES)
                    {
                        if(!buffer->line_vertex_buffer && !buffer->line_buffer)
                        {
                            buffer->line_vertex_buffer = c_arena_push_array(&render_state->persistant_arena, vertex_t,      MAX_VERTICES);
                            buffer->line_buffer        = c_arena_push_array(&render_state->persistant_arena, render_line_t, MAX_LINES);
                        }
                        Assert(buffer->line_vertex_buffer);
                        Assert(buffer->line_buffer);

                        result = buffer;
                        goto r_geometry_buffer_found;
                    }
                }break;
                default: {InvalidCodePath;}break;
            }
        }

        last_buffer = buffer;
    }

    if(!result)
    {
        result = c_arena_push_struct(&render_state->persistant_arena, geometry_buffer_t);
        last_buffer->next_buffer = result;
    }
r_geometry_buffer_found:
    Assert(result);
    return(result);
}

internal void
r_render_group_process_quad_geometry(geometry_buffer_t *buffer)
{
    DEBUG_TIMED_BLOCK();

    Assert(buffer->is_valid);
    Assert(buffer->quad_count < MAX_QUADS);

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
        *bottom_right = quad->bottom_right;
        *bottom_left  = quad->bottom_left;

        buffer->quad_vertex_count += 4;
        Assert(buffer->quad_vertex_count < MAX_VERTICES);
    }
}

internal void
r_render_group_process_line_geometry(geometry_buffer_t *buffer)
{
    DEBUG_TIMED_BLOCK();

    Assert(buffer->is_valid);
    Assert(buffer->line_count < MAX_LINES);

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
    for(geometry_buffer_t *buffer = &render_group->first_buffer;
        buffer;
        buffer = buffer->next_buffer)
    {
        buffer->line_vertex_count = 0;
        buffer->quad_vertex_count = 0;

        if(buffer->quad_count > 0)
        {
            // s_work_queue_add_entry(queue,
            //                        (work_queue_callback_t*)r_render_group_process_quad_geometry,
            //                        (void*)buffer);

            r_render_group_process_quad_geometry(buffer);
        }

        if(buffer->line_count > 0)
        {
            // s_work_queue_add_entry(queue,
            //                        (work_queue_callback_t*)r_render_group_process_line_geometry,
            //                        (void*)buffer);

            r_render_group_process_line_geometry(buffer);
        }
    }
}

internal render_group_t* 
r_render_group_create_new(render_state_t *render_state, hash_table_t *hash_table, u64 combo_id)
{
    DEBUG_TIMED_BLOCK();

    render_state->render_group_counter++;

    render_group_t *result = c_arena_push_struct(&render_state->persistant_arena, render_group_t);
    result->group_ID    = combo_id;
    result->render_desc = (render_group_desc_t){
        .render_material = render_state->draw_frame.active_material,
        .render_phase    = render_state->draw_frame.active_render_phase,
        .pipeline_state  = render_state->pipeline_state,
    };

    u64 index = combo_id;
    Assert(index >= 0);

    result->phase_idx = index;

    hash_table_entry *entry = hash_table->entries + index;
    entry->value = result;

    hash_table->entry_counter++;
    return(result);
}

internal inline void
r_renderpass_begin(render_state_t *render_state)
{
    DEBUG_TIMED_BLOCK();

    draw_frame_data_t *draw_frame = &render_state->draw_frame;

    // TODO(Sleepster): Right now, this is wrong. Material ID is always 0
    u32 material_id  = draw_frame->active_material.material_ID;
    u32 render_phase = draw_frame->active_render_phase;
    
    render_phase_data_t *render_phase_data = null;
    if(render_phase == RGP_Preblit)  render_phase_data  = &render_state->preblit_phase;
    if(render_phase == RGP_Postblit) render_phase_data = &render_state->postblit_phase;
    Assert(render_phase_data != null);

    render_group_container_t *container = null;
    if(render_state->pipeline_state.blending) container = &render_phase_data->transparent;
    else                                      container = &render_phase_data->opaque;
    Assert(container != null);

    u64 hash = default_fnv_hash_value;
    hash = c_fnv_hash_value((byte*)&material_id,    sizeof(u32), hash);
    hash = c_fnv_hash_value((byte*)&render_phase,   sizeof(u32), hash);
    hash = c_fnv_hash_value((byte*)&render_state->pipeline_state, sizeof(render_pipeline_state_t), hash);

    u64 combo_id = hash % container->group_hash.max_entries;
    render_group_t *render_group = (render_group_t*)c_hash_get_value_from_raw_index(&container->group_hash, combo_id);
    if(!render_group)
    {
        render_group = r_render_group_create_new(render_state, &container->group_hash, combo_id);

        u32 render_group_index = container->render_group_id_counter++;
        container->render_group_ids[render_group_index] = render_group->phase_idx;
    }
    Assert(render_group != null);

    bool8 is_found = false;
    for(u32 id_index = 0;
        id_index < container->render_groups_used_this_frame;
        ++id_index)
    {
        u32 id = container->ids_used_this_frame[id_index];
        if(id == render_group->group_ID)
        {
            is_found = true;
            break;
        }
    }

    if(!is_found)
    {
        container->ids_used_this_frame[container->render_groups_used_this_frame] = render_group->group_ID;
        container->render_groups_used_this_frame += 1;
    }

    draw_frame->active_render_group = render_group;
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

    // TODO(Sleepster): Is this really where we want this too live?
    at_atlas_handler_build_atlas(asset_manager, &asset_manager->texture_catalog.primary_handler);

    for(u32 group_index = 0;
        group_index < render_state->preblit_phase.opaque.render_groups_used_this_frame;
        ++group_index)
    {
        u32 hash_index = render_state->preblit_phase.opaque.ids_used_this_frame[group_index];

        render_group_t *render_group = (render_group_t*)render_state->preblit_phase.opaque.group_hash.entries[hash_index].value; 
        Assert(render_group);

        r_render_group_handle_geometry_buffers(&asset_manager->queue_manager->high_priority_queue, render_group);
    }

    for(u32 group_index = 0;
        group_index < render_state->preblit_phase.transparent.render_groups_used_this_frame;
        ++group_index)
    {
        u32 hash_index = render_state->preblit_phase.transparent.ids_used_this_frame[group_index];

        render_group_t *render_group = (render_group_t*)render_state->preblit_phase.transparent.group_hash.entries[hash_index].value; 
        Assert(render_group);

        r_render_group_handle_geometry_buffers(&asset_manager->queue_manager->high_priority_queue, render_group);
    }

    for(u32 group_index = 0;
        group_index < render_state->postblit_phase.opaque.render_groups_used_this_frame;
        ++group_index)
    {
        u32 hash_index = render_state->postblit_phase.opaque.ids_used_this_frame[group_index];

        render_group_t *render_group = (render_group_t*)render_state->postblit_phase.opaque.group_hash.entries[hash_index].value; 
        Assert(render_group);

        r_render_group_handle_geometry_buffers(&asset_manager->queue_manager->high_priority_queue, render_group);
    }

    for(u32 group_index = 0;
        group_index < render_state->postblit_phase.transparent.render_groups_used_this_frame;
        ++group_index)
    {
        u32 hash_index = render_state->postblit_phase.transparent.ids_used_this_frame[group_index];

        render_group_t *render_group = (render_group_t*)render_state->postblit_phase.transparent.group_hash.entries[hash_index].value; 
        Assert(render_group);

        r_render_group_handle_geometry_buffers(&asset_manager->queue_manager->high_priority_queue, render_group);
    }

    s_work_queue_finish_all_work(&asset_manager->queue_manager->high_priority_queue);
}
