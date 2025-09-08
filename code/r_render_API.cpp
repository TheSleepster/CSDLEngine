/* ========================================================================
   $File: r_render_API.c $
   $Date: Sat, 26 Jul 25: 04:51PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "r_render_API.h"

internal void
r_add_texture_to_texture_list(render_group_t *group, u32 textureID)
{
    for(u32 ID_index = 0;
        ID_index < group->desired_texture_count;
        ++ID_index)
    {
        u32 ID = group->textureIDs[ID_index];
        if(ID == textureID) return;
    }
    Assert(group->desired_texture_count + 1 < MAX_TEXTURES);

    group->textureIDs[group->desired_texture_count] = textureID;
    group->desired_texture_count += 1;
}

internal render_quad_t
r_create_render_quad(render_state_t       *render_state,
                     vec2_t                position,
                     vec2_t                render_size,
                     vec4_t                color,
                     float32               rotation,
                     vec2_t                texture_offset,
                     vec2_t                texture_size,
                     u32                   gpu_texture_id,
                     render_quad_options_t render_options)
{
    DEBUG_TIMED_BLOCK();
    Assert(render_state->draw_frame.active_render_group->render_desc.primitive_type == RGPT_Quads);

    render_quad_t result = {};
    result.options = render_options;

    float32 top    = position.y;
    float32 left   = position.x;
    float32 bottom = position.y + render_size.y;
    float32 right  = position.x + render_size.x;

    render_group_desc_t *render_group_data = &render_state->draw_frame.active_render_group->render_desc;

    /* NOTE(Sleepster): We are using an Orthographic projection matrix.
     * In OpenGL and Vulkan, the depth values are always normalized between -1 and 1
     */
    float32 near_value = -1;
    float32 far_value  =  1;

    float32 depth_step        = (far_value - near_value) / MAX_RENDER_LAYERS;
    float32 layer_depth_value = near_value  + (render_group_data->render_layer * depth_step);

    result.layer_depth            = layer_depth_value;
    result.top_left.vPosition     = vec3_create_float(left, top,     layer_depth_value);
    result.top_right.vPosition    = vec3_create_float(right, top,    layer_depth_value);
    result.bottom_left.vPosition  = vec3_create_float(left, bottom,  layer_depth_value);
    result.bottom_right.vPosition = vec3_create_float(right, bottom, layer_depth_value);

    vec4_t min_clip_pos = vec3_expand_vec4(result.top_left.vPosition, 1);
    vec4_t max_clip_pos = vec3_expand_vec4(result.bottom_right.vPosition, 1);

    mat4_t clip_mat = mat4_multiply(render_group_data->view_matrix, render_group_data->projection_matrix);
    
    vec4_t quad_min = vec4_transform(clip_mat, min_clip_pos);
    vec4_t quad_max = vec4_transform(clip_mat, max_clip_pos);
    result.culled = ((quad_max.x < -1.0f) || (quad_min.x > 1.0f) ||
                     (quad_max.y < -1.0f) || (quad_min.y > 1.0f));
    if(!result.culled)
    {
        if(rotation > 0)
        {
            result.top_left.vPosition     = vec2_expand_vec3(vec2_rotate(result.top_left.vPosition.xy,     DegToRad(rotation)), layer_depth_value);
            result.top_right.vPosition    = vec2_expand_vec3(vec2_rotate(result.top_right.vPosition.xy,    DegToRad(rotation)), layer_depth_value);
            result.bottom_left.vPosition  = vec2_expand_vec3(vec2_rotate(result.bottom_left.vPosition.xy,  DegToRad(rotation)), layer_depth_value);
            result.bottom_right.vPosition = vec2_expand_vec3(vec2_rotate(result.bottom_right.vPosition.xy, DegToRad(rotation)), layer_depth_value);
        }

        if(gpu_texture_id != MAX_U32)
        {
            for(u32 index = 0;
                index < 4;
                ++index)
            {
                result.elements[index].vTextureIndex = gpu_texture_id;
            }

            vec2_t uv_min =  texture_offset;
            vec2_t uv_max =  vec2_add(texture_offset, texture_size);

            result.top_left.vUVData     = uv_min;
            result.top_right.vUVData    = vec2_create_float(uv_max.x, uv_min.y);
            result.bottom_left.vUVData  = vec2_create_float(uv_min.x, uv_max.y);
            result.bottom_right.vUVData = uv_max;

            r_add_texture_to_texture_list(render_state->draw_frame.active_render_group, gpu_texture_id);
        }
        else
        {
            for(u32 index = 0;
                index < 4;
                ++index)
            {
                result.elements[index].vTextureIndex = MAX_U32;
            }
        }

        for(u32 index = 0;
            index < 4;
            ++index)
        {
            result.elements[index].vColor = color;
        }
    }

    return(result);
}

internal render_quad_t*
r_draw_texture_ex(render_state_t       *render_state,
                  vec2_t                position,
                  vec2_t                render_size,
                  vec4_t                color,
                  float32               rotation,
                  vec2_t                texture_offset,
                  vec2_t                texture_size,
                  u32                   gpu_texture_id,
                  render_quad_options_t render_options)
{
    Assert(render_state->draw_frame.active_render_group != null);
    render_quad_t  *result = null;

    render_quad_t quad_init = r_create_render_quad(render_state,
                                                   position,
                                                   render_size,
                                                   color,
                                                   rotation,
                                                   texture_offset,
                                                   texture_size,
                                                   gpu_texture_id,
                                                   render_options);
    if(!quad_init.culled)
    {
        render_group_t *active_render_group = render_state->draw_frame.active_render_group;
        
        active_render_group->quad_buffer[active_render_group->quad_count] = quad_init;

        result = &active_render_group->quad_buffer[active_render_group->quad_count];
        if(render_options & RQO_SHADOWCASTER)
        {
            if(!render_state->draw_frame.shadow_casters)
            {
                render_state->draw_frame.shadow_casters = c_arena_push_array(&render_state->draw_frame_arena, shadow_caster2D_t, MAX_QUADS);
            }

            shadow_caster2D_t caster = {};
            caster.quad_data = active_render_group->quad_buffer[active_render_group->quad_count];

            render_state->draw_frame.shadow_casters[render_state->draw_frame.shadow_caster_counter] = caster; 
            render_state->draw_frame.shadow_caster_counter += 1;
        }

        active_render_group->quad_count += 1;
    }

    return(result);
}

internal inline render_quad_t*
r_draw_texture(render_state_t       *render_state,
               vec2_t                position,
               vec2_t                render_size,
               vec4_t                color,
               float32               rotation,
               asset_handle_t        texture_handle,
               render_quad_options_t render_options)
{
    render_quad_t *result = null;
    
    vec2_t uv_min     = vec2();
    vec2_t uv_max     = vec2();
    u32    texture_id = MAX_U32;
    if(texture_handle.is_valid)
    {
        uv_min     = *texture_handle.texture->uv_min;
        uv_max     = *texture_handle.texture->uv_max;
        texture_id =  texture_handle.texture->GPU_textureID;
    }

    result = r_draw_texture_ex(render_state,
                                  position,
                                  render_size,
                                  color,
                                  rotation,
                                  uv_min,
                                  uv_max,
                                  texture_id,
                                  render_options);
    return(result);
}

internal render_quad_t*
r_draw_rect(render_state_t       *render_state,
            vec2_t                position,
            vec2_t                render_size,
            vec4_t                color,
            float32               rotation,
            render_quad_options_t render_options)
{
    asset_handle_t invalid_handle = {};
    render_quad_t *result = r_draw_texture(render_state,
                                           position,
                                           render_size,
                                           color,
                                           rotation,
                                           invalid_handle,
                                           render_options);

    return(result);
}

internal s32
r_prepare_string_for_rendering(asset_manager_t *asset_manager, dynamic_render_font_varient_t *varient, string_t output)
{
    s32 result = 0;
    for(u8 *p_character = output.data;
        p_character < output.data + output.count;
        p_character = unicode_next_character(p_character))
    {
        font_glyph_t *glyph = s_asset_font_get_utf8_glyph(asset_manager, varient, p_character);
        result += glyph->glyph_render_size.x + glyph->advance;

        if(glyph->owner_page->bitmap_dirty)
        {
            texture2D_t *texture = &glyph->owner_page->font_atlas;
            if(texture->view->GPU_textureID == 0)
            {
                r_texture_make_gpu(texture, texture->has_AA, texture->filter_type);
            }
            else
            {
                r_texture_update_from_bitmap(asset_manager, texture);
            }

            glyph->owner_page->bitmap_dirty = false;
        }
    }

    return(result);
}

internal void
r_draw_string(asset_manager_t       *asset_manager,
              render_state_t        *render_state,
              string_t               output,
              asset_handle_t         font,
              u32                    pixel_size,
              vec2_t                 position,
              vec4_t                 color,
              render_quad_options_t  render_options)
{
    DEBUG_TIMED_BLOCK();

    dynamic_render_font_varient_t *varient = s_asset_font_get_at_size(asset_manager, font, pixel_size);
    if(varient)
    {
        r_prepare_string_for_rendering(asset_manager, varient, output);
        
        vec2_t draw_position = position;
        for(u8 *p_character = output.data;
            p_character < output.data + output.count;
            p_character = unicode_next_character(p_character))
        {
            u8  character   = *p_character;
            if(character == '\0') break;
            
            if(character == '\n' || character == '\r')
            {
                draw_position.x  = position.x;
                draw_position.y -= varient->line_spacing;

                continue;
            }

            font_glyph_t *glyph = s_asset_font_get_utf8_glyph(asset_manager, varient, p_character);
            if(character == '\t' || character == ' ')
            {
                draw_position.x += glyph->advance;
            }
            else
            {
                r_draw_texture_ex(render_state,
                                  vec2_create_float(floorf(draw_position.x + glyph->offset_x),
                                                    floorf(draw_position.y - glyph->offset_y)),
                                  glyph->glyph_render_size,
                                  color,
                                  0.0f,
                                  glyph->atlas_offset,
                                  glyph->glyph_size,
                                  glyph->owner_page->font_atlas.view->GPU_textureID,
                                  render_options);

                draw_position.x += glyph->advance;
            }
        }
    }
    else
    {
        log_error("Could not get a varient of your font: '%s' at the size of: '%d'...\n", font.font->filename.data, pixel_size);
    }
}

internal render_line_t* 
r_create_render_line(render_state_t *render_state, 
                     vec2_t          start_point,
                     vec2_t          end_point,
                     float32         thickness,
                     vec4_t          color)
{
    render_line_t *result = null;
    render_group_t *active_group = render_state->draw_frame.active_render_group;
    Assert(active_group->render_desc.primitive_type == RGPT_Lines);
    
    if(!active_group->line_buffer)
    {
        active_group->line_buffer = c_arena_push_array(&render_state->draw_frame_arena, render_line_t, MAX_LINES);
    }
    float32 near_value = -1;
    float32 far_value  =  1;

    float32 depth_step        = (far_value - near_value) / MAX_RENDER_LAYERS;
    float32 layer_depth_value = near_value  + (active_group->render_desc.render_layer * depth_step);

    render_line_t new_line = {};
    new_line.layer_depth           = layer_depth_value;
    new_line.start_point.vPosition = vec2_expand_vec3(start_point, layer_depth_value);
    new_line.end_point.vPosition   = vec2_expand_vec3(end_point, layer_depth_value);
    new_line.start_point.vColor    = color;
    new_line.end_point.vColor      = color;

    new_line.start_point.vTextureIndex = MAX_U32;
    new_line.end_point.vTextureIndex   = MAX_U32;

     result = active_group->line_buffer + active_group->line_count++;
    *result = new_line;

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
r_build_renderpass_desc(GPU_shader_t                     *desired_shader,
                        u32                               render_layer,
                        mat4_t                            view_matrix,
                        mat4_t                            projection_matrix,
                        render_group_effects_t            render_effects,
                        render_group_desired_render_phase render_phase,
                        render_group_primitive_type_t     primitive_type,
                        bool8                             supports_transparency)
{
    DEBUG_TIMED_BLOCK();
    Assert(render_layer <= MAX_RENDER_LAYERS);
    
    render_group_desc_t result;
    result.shader                = desired_shader;
    result.render_layer          = render_layer;
    result.view_matrix           = view_matrix;
    result.projection_matrix     = projection_matrix;
    result.desired_effects       = render_effects;
    result.desired_phase         = render_phase;
    result.primitive_type        = primitive_type;
    result.supports_transparency = supports_transparency;

    return(result);
}

internal u64
r_get_renderpass_desc_id(render_group_desc_t *render_pass_desc)
{
    u64 result = 0;
    u64 hash_value = 14695981039346656037ULL;

    hash_value = c_fnv_hash_value((u8*) render_pass_desc->shader,            sizeof(GPU_shader_t*),                       hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->desired_effects,   sizeof(render_group_effects_t),              hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->desired_phase,     sizeof(render_group_desired_render_phase_t), hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->primitive_type,    sizeof(render_group_primitive_type_t),       hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->render_layer,      sizeof(u32),                                 hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->view_matrix,       sizeof(mat4_t),                              hash_value);
    hash_value = c_fnv_hash_value((u8*)&render_pass_desc->projection_matrix, sizeof(mat4_t),                              hash_value);

    result = hash_value;
    return(result);
}

internal render_group_t* 
r_begin_renderpass(render_state_t *render_state, render_group_desc_t *render_pass_desc)
{
    DEBUG_TIMED_BLOCK();
    if(!render_state->draw_frame.is_initialized)
    {
        render_state->draw_frame.preblit_pass_data.opaque_render_groups       = (render_group_t **)c_arena_push_size(&render_state->draw_frame_arena, sizeof(render_group_t*) * MAX_RENDER_GROUPS);
        render_state->draw_frame.preblit_pass_data.transparent_render_groups  = (render_group_t **)c_arena_push_size(&render_state->draw_frame_arena, sizeof(render_group_t*) * MAX_RENDER_GROUPS);

        render_state->draw_frame.postblit_pass_data.opaque_render_groups      = (render_group_t **)c_arena_push_size(&render_state->draw_frame_arena, sizeof(render_group_t*) * MAX_RENDER_GROUPS);
        render_state->draw_frame.postblit_pass_data.transparent_render_groups = (render_group_t **)c_arena_push_size(&render_state->draw_frame_arena, sizeof(render_group_t*) * MAX_RENDER_GROUPS);

        render_state->draw_frame.is_initialized   = true;
    }

    Assert(render_pass_desc->render_layer <= MAX_RENDER_LAYERS);
    u32             *render_group_counter_ptr = null;
    render_group_t **render_group_array       = null;

    render_phase_data_t *render_phase_data = null;
    switch(render_pass_desc->desired_phase)
    {
        case RGP_MainGamePass: render_phase_data = &render_state->draw_frame.preblit_pass_data;  break;
        case RGP_PostBlitPass: render_phase_data = &render_state->draw_frame.postblit_pass_data; break;
        default: InvalidCodePath; break;
    }
    Assert(render_phase_data);

    if(render_pass_desc->supports_transparency)
    {
        render_group_array       =  render_phase_data->transparent_render_groups;
        render_group_counter_ptr = &render_phase_data->transparent_render_group_counter;
    }
    else
    {
        render_group_array       =  render_phase_data->opaque_render_groups;
        render_group_counter_ptr = &render_phase_data->opaque_render_group_counter;
    }
    u32 render_group_count = *render_group_counter_ptr;

    Assert(render_group_array       != null);
    Assert(render_group_counter_ptr != null);
    
    u64 pass_id = r_get_renderpass_desc_id(render_pass_desc);
    render_group_t *active_group = null;
    for(u32 pass_index = 0;
        pass_index < render_group_count;
        ++pass_index)
    {
        render_group_t *found = render_group_array[pass_index];
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
        active_group->quad_buffer   = c_arena_push_array(&render_state->draw_frame_arena, render_quad_t, MAX_QUADS);
        active_group->vertex_buffer = c_arena_push_array(&render_state->draw_frame_arena, vertex_t,      MAX_VERTICES);
        Assert(active_group != null);

        render_group_array[render_group_count] = active_group;
        Assert(render_group_array[render_group_count] != null);
        *render_group_counter_ptr += 1;
    }

    render_state->draw_frame.active_render_group = active_group;
    Assert(render_state->draw_frame.active_render_group != null);

    return(render_state->draw_frame.active_render_group);
}

internal inline void
r_end_renderpass(render_state_t *render_state)
{
    Assert(render_state->draw_frame.active_render_group != null);
    render_state->draw_frame.active_render_group = null;
}

internal void
r_fill_render_group_vertex_buffer(render_group_t *render_group)
{
    DEBUG_TIMED_BLOCK();
    switch(render_group->render_desc.primitive_type)
    {
        case RGPT_Quads:
        {
            for(u32 quad_index = 0;
                quad_index < render_group->quad_count;
                ++quad_index)
            {
                render_quad_t *quad  = render_group->quad_buffer   + quad_index;
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
        }break;
        case RGPT_Lines:
        {
            for(u32 line_index = 0;
                line_index < render_group->line_count;
                ++line_index)
            {
                render_line_t *line  = render_group->line_buffer   + line_index;
                vertex_t *buffer_ptr = render_group->vertex_buffer + render_group->vertex_count;

                vertex_t *starting_point = buffer_ptr + 0;
                vertex_t *ending_point   = buffer_ptr + 1;

                *starting_point = line->start_point;
                *ending_point   = line->end_point;

                render_group->vertex_count += 2;
            }
        }break;
        default: {InvalidCodePath;}break;
    }
}

// TODO(Sleepster): MULTITHREAD THIS
internal void
r_handle_renderpass_data(asset_manager_t *asset_manager, render_state_t *render_state)
{
    DEBUG_TIMED_BLOCK();
    // TODO(Sleepster): Is this really where we want this too live?
    at_atlas_handler_build_atlas(asset_manager, &asset_manager->texture_catalog.primary_handler);
    draw_frame_t *draw_frame = &render_state->draw_frame;
    
    // NOTE(Sleepster): PREBLIT GROUP SETUP 
    {
        if(draw_frame->preblit_pass_data.opaque_render_group_counter > 0)
        {
            for(u32 render_group_idx = 0;
                render_group_idx < draw_frame->preblit_pass_data.opaque_render_group_counter;
                ++render_group_idx)
            {
                render_group_t *render_group = (render_group_t*)draw_frame->preblit_pass_data.opaque_render_groups[render_group_idx];
                r_fill_render_group_vertex_buffer(render_group);
            }
        }

        if(draw_frame->preblit_pass_data.transparent_render_group_counter > 0)
        {
            render_group_t **sorted_layer_buffer = c_arena_push_array(&render_state->draw_frame_arena,
                                                                      render_group_t*,
                                                                      draw_frame->preblit_pass_data.transparent_render_group_counter);
            c_radix_sort(draw_frame->preblit_pass_data.transparent_render_groups,
                         sorted_layer_buffer,
                         draw_frame->preblit_pass_data.transparent_render_group_counter,
                         sizeof(render_group_t*),
                         IntFromPtr(OffsetOf(render_group_t, render_desc.render_layer)),
                         8);
            for(u32 render_group_idx = 0;
                render_group_idx < draw_frame->preblit_pass_data.transparent_render_group_counter;
                ++render_group_idx)
            {
                render_group_t  *render_group = (render_group_t*)draw_frame->preblit_pass_data.transparent_render_groups[render_group_idx];
                r_fill_render_group_vertex_buffer(render_group);
            }
        }
    }

    // NOTE(Sleepster): POSTBLIT GROUP SETUP 
    {
        if(draw_frame->postblit_pass_data.opaque_render_group_counter > 0)
        {
            for(u32 render_group_idx = 0;
                render_group_idx < draw_frame->postblit_pass_data.opaque_render_group_counter;
                ++render_group_idx)
            {
                render_group_t *render_group = (render_group_t*)draw_frame->postblit_pass_data.opaque_render_groups[render_group_idx];
                r_fill_render_group_vertex_buffer(render_group);
            }
        }

        if(draw_frame->postblit_pass_data.transparent_render_group_counter > 0)
        {
            render_group_t **sorted_layer_buffer = c_arena_push_array(&render_state->draw_frame_arena,
                                                                      render_group_t*,
                                                                      draw_frame->postblit_pass_data.transparent_render_group_counter);
            c_radix_sort(draw_frame->postblit_pass_data.transparent_render_groups,
                         sorted_layer_buffer,
                         draw_frame->postblit_pass_data.transparent_render_group_counter,
                         sizeof(render_group_t*),
                         IntFromPtr(OffsetOf(render_group_t, render_desc.render_layer)),
                         8);
            for(u32 render_group_idx = 0;
                render_group_idx < draw_frame->postblit_pass_data.transparent_render_group_counter;
                ++render_group_idx)
            {
                render_group_t  *render_group = (render_group_t*)draw_frame->postblit_pass_data.transparent_render_groups[render_group_idx];
                r_fill_render_group_vertex_buffer(render_group);
            }
        }
    }
}
