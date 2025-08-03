/* ========================================================================
   $File: r_render_API.c $
   $Date: Sat, 26 Jul 25: 04:51PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "r_renderer_data.h"

internal render_quad_t
r_create_render_quad(vec2_t                position,
                     vec2_t                render_size,
                     vec4_t                color,
                     float32               rotation,
                     render_quad_options_t render_options)
{
    render_quad_t result;

    float32 top    = position.y;
    float32 left   = position.x;
    float32 bottom = position.y + render_size.y;
    float32 right  = position.x + render_size.y;

    result.options = render_options;

    if(rotation > 0)
    {
        result.top_left.vPosition     = vec2_rotate(vec2_create_float(left, top),     DegToRad(rotation));
        result.top_right.vPosition    = vec2_rotate(vec2_create_float(right, top),    DegToRad(rotation));
        result.bottom_left.vPosition  = vec2_rotate(vec2_create_float(left, bottom),  DegToRad(rotation));
        result.bottom_right.vPosition = vec2_rotate(vec2_create_float(right, bottom), DegToRad(rotation));
    }
    else
    {
        result.top_left.vPosition     = vec2_create_float(left, top);
        result.top_right.vPosition    = vec2_create_float(right, top);
        result.bottom_left.vPosition  = vec2_create_float(left, bottom);
        result.bottom_right.vPosition = vec2_create_float(right, bottom);
    }

    for(u32 index = 0;
        index < 4;
        ++index)
    {
        result.elements[index].vColor = color;
    }

    return(result);
}

internal render_quad_t*
r_draw_quad(render_state_t       *render_state,
            vec2_t                position,
            vec2_t                render_size,
            vec4_t                color,
            float32               rotation,
            render_quad_options_t render_options)
{
    Assert(render_state->draw_frame.active_render_group != null);
    
    render_quad_t  *result;
    render_group_t *active_render_group = render_state->draw_frame.active_render_group;
    active_render_group->buffer_quads[active_render_group->quad_count] = r_create_render_quad(position,
                                                                                              render_size,
                                                                                              color,
                                                                                              rotation,
                                                                                              render_options);
    result = &active_render_group->buffer_quads[active_render_group->quad_count];
    if(render_options & RQO_SHADOWCASTER)
    {
        if(!render_state->draw_frame.shadow_casters)
        {
            render_state->draw_frame.shadow_casters = c_arena_push_array(&render_state->draw_frame_arena, shadow_caster2D_t, MAX_QUADS);
        }

        shadow_caster2D_t caster = {};
        caster.quad_data = active_render_group->buffer_quads[active_render_group->quad_count];

        render_state->draw_frame.shadow_casters[render_state->draw_frame.shadow_caster_counter] = caster; 
        render_state->draw_frame.shadow_caster_counter += 1;
    }

    active_render_group->quad_count += 1;
    return(result);
}


////////////////////
// LIGHTING
////////////////////
internal point_light_t*
r_create_point_light(render_state_t *render_state, vec2_t position, vec4_t color, float32 radius)
{
    point_light_t *result = null;
    if(!render_state->draw_frame.point_lights)
    {
        render_state->draw_frame.point_lights = c_arena_push_array(&render_state->draw_frame_arena, point_light_t, MAX_LIGHTS);
    }
    Assert(render_state->draw_frame.point_lights);
    result = render_state->draw_frame.point_lights + render_state->draw_frame.light_counter;

    result->ws_position = position;
    result->light_color = color;
    result->radius      = radius;

    return(result);
}

internal void
r_handle_lighting_data(render_state_t *render_state)
{
/*
  What do we want this function to do?
  LIGHTING:
  - Compute the correct UVs into the light atlas for all the lights in the current scene
  - Convert a light_atlas_cell_index -> UV coords
  - Assign a vector4 light mask for the channel the light occupies
  - Channel index 0 = .{1, 0, 0, 0}. Channel index 1 = .{0, 1, 0, 0}...
  - Create clip space geometry to store the lighting information in the atlas
  - Create a quad that is made in world space coordinates, bring them to clip space
  - Add this quad to the light quads vertex buffer 
  SHADOWS: 
  - For each light, loop over each occluder to figure out if it is within the influence of the light
  - If the occluder IS within range, set it's cell_index and it's channel_index to that of the light's
  - If the occluder ISN'T then skip it
  - For each occluder generate a shadow quad and bring it's transformation space to that of clip space. 
  - Add the quad to the occluder shadow quads vertex buffer.
  DRAWING:
  - Render the light mask (occluder) geometry into the light map,
  using the cell_index (which converts to UV coords in the
  bigger lightmap) and color_mask to render it correctly the
  glColorMask() is set to to only write alpha.

  - Render all lights using their lightmap geometry into the
  light map, again using the cell_index and color mask to
  ensure proper writing.
*/
}

/////////////////////////////////
// RENDER GROUP
/////////////////////////////////


internal inline render_group_desc_t
r_build_renderpass_desc(GPU_shader_t          *desired_shader,
                         u32                    render_layer,
                         mat4_t                 view_matrix,
                         mat4_t                 projection_matrix,
                         render_group_effects_t render_effects)
{
    render_group_desc_t result;
    result.shader            = desired_shader;
    result.render_layer      = render_layer;
    result.view_matrix       = view_matrix;
    result.projection_matrix = projection_matrix;
    result.render_effects    = render_effects;

    return(result);
}

internal u64
r_get_renderpass_desc_id(render_group_desc_t *render_pass_desc)
{
    u64 result = 0;
    u64 hash_value = 14695981039346656037ULL;

    hash_value = c_fnv_hash_value((u8*)render_pass_desc->shader,             sizeof(GPU_shader_t*),          hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->render_effects,    sizeof(render_group_effects_t), hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->render_layer,      sizeof(u32),                    hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->view_matrix,       sizeof(mat4_t),                 hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->projection_matrix, sizeof(mat4_t),                 hash_value);

    result = hash_value;
    return(result);
}

internal void
r_begin_renderpass(render_state_t *render_state, render_group_desc_t *render_pass_desc)
{
    u64 pass_id = r_get_renderpass_desc_id(render_pass_desc);
    if(render_state->draw_frame.render_groups == null || !render_state->draw_frame.initialized)
    {
        render_state->draw_frame.render_groups = (render_group_t **)c_arena_push_size(&render_state->draw_frame_arena, sizeof(render_group_t*) * MAX_RENDER_GROUPS);
        render_state->draw_frame.initialized   = true;
    }
    Assert(render_state->draw_frame.render_groups != null);
    
    render_group_t *active_group = null;
    for(u32 pass_index = 0;
        pass_index < render_state->draw_frame.render_group_counter;
        ++pass_index)
    {
        render_group_t *found = render_state->draw_frame.render_groups[pass_index];
        u64 found_pass_id = r_get_renderpass_desc_id(&found->render_desc);
        if(found_pass_id == pass_id)
        {
            active_group = found;
            break;
        }
    }

    if(!active_group)
    {
        active_group = c_arena_push_struct(&render_state->draw_frame_arena, render_group_t);
        active_group->render_desc   = *render_pass_desc;
        active_group->buffer_quads  = c_arena_push_array(&render_state->draw_frame_arena, render_quad_t, MAX_QUADS);
        active_group->vertex_buffer = c_arena_push_array(&render_state->draw_frame_arena, vertex_t,      MAX_VERTICES);
        Assert(active_group != null);

        u32 old_layer_mask = render_state->draw_frame.render_layer_mask;
        render_state->draw_frame.render_layer_mask |= 1 << render_pass_desc->render_layer;
        if(old_layer_mask != render_state->draw_frame.render_layer_mask)
        {
            render_state->draw_frame.used_render_layers[render_state->draw_frame.used_render_layer_count] = render_pass_desc->render_layer;
            render_state->draw_frame.used_render_layer_count++;
        }

        render_state->draw_frame.render_groups[render_state->draw_frame.render_group_counter] = active_group;
        Assert(render_state->draw_frame.render_groups[render_state->draw_frame.render_group_counter] != null);

        render_state->draw_frame.render_group_counter += 1;
    }

    render_state->draw_frame.active_render_group = active_group;
    Assert(render_state->draw_frame.active_render_group != null);
}

internal inline void
r_end_renderpass(render_state_t *render_state)
{
    Assert(render_state->draw_frame.active_render_group != null);
    render_state->draw_frame.active_render_group = null;
}

internal void
r_handle_renderpass_data(render_state_t *render_state)
{
    render_group_t **sorted_layer_buffer = c_arena_push_array(&render_state->draw_frame_arena,
                                                              render_group_t*,
                                                              render_state->draw_frame.render_group_counter);
    c_radix_sort(render_state->draw_frame.render_groups,
                 sorted_layer_buffer,
                 render_state->draw_frame.render_group_counter,
                 sizeof(render_group_t*),
                 IntFromPtr(OffsetOf(render_group_t, render_desc.render_layer)),
                 8);
    
    // TODO(Sleepster): MULTITHREAD THIS
    for(u32 render_group_idx = 0;
        render_group_idx < render_state->draw_frame.render_group_counter;
        ++render_group_idx)
    {
        render_group_t *render_group = (render_group_t*)render_state->draw_frame.render_groups[render_group_idx];
        for(u32 quad_index = 0;
            quad_index < render_group->quad_count;
            ++quad_index)
        {
            render_quad_t *quad  = render_group->buffer_quads + quad_index;
            vertex_t *buffer_ptr = render_group->vertex_buffer + render_group->vertex_count;

            vertex_t *top_left     = buffer_ptr + 0;
            vertex_t *top_right    = buffer_ptr + 1;
            vertex_t *bottom_right = buffer_ptr + 2;
            vertex_t *bottom_left  = buffer_ptr + 3;

            *top_left     = quad->top_left;
            *top_right    = quad->top_right;
            *bottom_left  = quad->bottom_left;
            *bottom_right = quad->bottom_right;

            render_group->vertex_count += 4;
        }
    }
}
