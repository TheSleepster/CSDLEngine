/* ========================================================================
   $File: r_draw_interface.cpp $
   $Date: November 17 2025 04:57 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include "r_render_group.h"
#include "r_render_data.h"

///////////////////////////////
// RENDER STATE MODIFICATION
///////////////////////////////

internal inline void
r_set_active_render_material(render_state_t *render_state, render_material_t render_material)
{
    render_state->draw_frame.active_material = render_material;
}

internal inline void
r_set_active_render_camera(render_state_t *render_state, render_camera_t *render_camera)
{
    render_state->draw_frame.active_camera = render_camera;
}

internal inline void
r_set_active_render_layer(render_state_t *render_state, u32 layer)
{
    Assert(layer >= 0);
    Assert(layer <= MAX_RENDER_LAYERS);

    render_state->draw_frame.active_render_layer = layer;
}

internal inline void
r_set_active_render_phase(render_state_t *render_state, u32 render_phase)
{
    Assert(render_phase == RGP_Preblit || render_phase == RGP_Postblit);

    render_state->draw_frame.active_render_phase = (render_phase_t)render_phase;
}

internal inline void
r_set_active_blend_mode(render_state_t              *render_state,
                        render_group_blending_mode_t src_color_blend,
                        render_group_blending_mode_t dst_color_blend,
                        render_group_blending_mode_t src_alpha_blend,
                        render_group_blending_mode_t dst_alpha_blend)
{
    render_state->pipeline_state.src_color_blend_mode = src_color_blend;
    render_state->pipeline_state.dst_color_blend_mode = dst_color_blend;

    render_state->pipeline_state.src_alpha_blend_mode = src_alpha_blend;
    render_state->pipeline_state.dst_alpha_blend_mode = dst_alpha_blend;
}

internal inline void
r_set_active_blending_eqs(render_state_t *render_state,
                          render_group_blending_equation_t color_blend_eq,
                          render_group_blending_equation_t alpha_blend_eq)
{
    render_state->pipeline_state.color_blend_eq = color_blend_eq;
    render_state->pipeline_state.alpha_blend_eq = alpha_blend_eq;
}

internal inline void
r_set_active_blending_state(render_state_t *render_state, bool32 blending)
{
    render_state->pipeline_state.blending = blending;
}

internal inline void
r_set_active_depth_state(render_state_t *render_state, bool32 depth_test, bool32 depth_mask)
{
    render_state->pipeline_state.depth_testing = depth_test;
    render_state->pipeline_state.depth_writing = depth_mask;
}

internal inline void
r_set_active_depth_func(render_state_t *render_state, render_group_depth_function_t depth_func)
{
    render_state->pipeline_state.depth_func = depth_func;
}

internal void
r_pipeline_state_reset(render_state_t *render_state)
{
    DEBUG_TIMED_BLOCK();

    render_pipeline_state_t *pipeline_state = &render_state->pipeline_state;
    pipeline_state->src_color_blend_mode = RGBM_One;
    pipeline_state->dst_color_blend_mode = RGBM_Zero;
    pipeline_state->src_alpha_blend_mode = RGBM_One;
    pipeline_state->dst_alpha_blend_mode = RGBM_Zero;
    pipeline_state->color_blend_eq       = RGBE_Add;
    pipeline_state->alpha_blend_eq       = RGBE_Add;
    pipeline_state->depth_func           = RGDF_Greater;
    pipeline_state->depth_testing        = true;
    pipeline_state->depth_writing        = true;
    pipeline_state->blending             = false;

    pipeline_state->scissor_enabled = false;
    pipeline_state->scissor_x       = 0;
    pipeline_state->scissor_y       = 0;
    pipeline_state->scissor_w       = 0;
    pipeline_state->scissor_h       = 0;

    pipeline_state->render_line_width = 1.0f;
}

//////////////////////////
// PRIMITIVE RENDERING
//////////////////////////

internal render_quad_t
r_create_render_quad(render_state_t *render_state,
                     vec2_t          position,
                     vec2_t          render_size,
                     vec4_t          color,
                     float32         rotation,
                     vec2_t          texture_offset,
                     vec2_t          texture_size,
                     u32             render_options)
{
    DEBUG_TIMED_BLOCK();

    render_quad_t result = {};
    result.options = (render_quad_options_t)render_options;

    float32 top    = position.y;
    float32 left   = position.x;
    float32 bottom = position.y + render_size.y;
    float32 right  = position.x + render_size.x;

    /* NOTE(Sleepster): We are using an Orthographic projection matrix.
     * In OpenGL and Vulkan, the depth values are always normalized between -1 and 1
     */
    float32 near_value = -1;
    float32 far_value  =  1;

    float32 depth_step        = (far_value - near_value) / MAX_RENDER_LAYERS;
    float32 layer_depth_value = near_value + (render_state->draw_frame.active_render_layer * depth_step);

    result.layer_depth            = layer_depth_value;
    result.top_left.vPosition     = vec3(left, top,     layer_depth_value);
    result.top_right.vPosition    = vec3(right, top,    layer_depth_value);
    result.bottom_left.vPosition  = vec3(left, bottom,  layer_depth_value);
    result.bottom_right.vPosition = vec3(right, bottom, layer_depth_value);

    vec4_t min_clip_pos = vec3_expand_vec4(result.top_left.vPosition, 1);
    vec4_t max_clip_pos = vec3_expand_vec4(result.bottom_right.vPosition, 1);

    mat4_t clip_mat = mat4_multiply(render_state->draw_frame.active_camera->view_matrix, 
                                    render_state->draw_frame.active_camera->projection_matrix);
    
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

        if(render_state->draw_frame.active_material.texture != null)
        {
            vec2_t uv_min =  texture_offset;
            vec2_t uv_max =  vec2_add(texture_offset, texture_size);

            result.top_left.vUVData     = uv_min;
            result.top_right.vUVData    = vec2(uv_max.x, uv_min.y);
            result.bottom_left.vUVData  = vec2(uv_min.x, uv_max.y);
            result.bottom_right.vUVData = uv_max;
        }

        if(render_options & RQO_UNTEXTURED)
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
r_draw_texture_ex(render_state_t *render_state,
                  vec2_t          position,
                  vec2_t          render_size,
                  vec4_t          color,
                  float32         rotation,
                  vec2_t          texture_offset,
                  vec2_t          texture_size,
                  u32             render_options)
{
    DEBUG_TIMED_BLOCK();
    Assert(render_state->draw_frame.active_render_group != null);
    render_quad_t  *result = null;

    render_quad_t quad_init = r_create_render_quad(render_state,
                                                   position,
                                                   render_size,
                                                   color,
                                                   rotation,
                                                   texture_offset,
                                                   texture_size,
                                                   render_options);
    if(!quad_init.culled)
    {
        geometry_buffer_t *g_buffer = r_render_group_get_buffer(render_state, 
                                                                render_state->draw_frame.active_render_group, 
                                                                RGPT_Quads);
        if(g_buffer->is_valid)
        {
            g_buffer->quad_buffer[g_buffer->quad_count] = quad_init;
            result = g_buffer->quad_buffer + g_buffer->quad_count;
#if 0
            if(render_options & RQO_SHADOWCASTER)
            {
                // TODO(Sleepster): Why is this here? 
                if(!render_state->draw_frame.shadow_casters)
                {
                    render_state->draw_frame.shadow_casters = c_arena_push_array(&render_state->draw_frame_arena, shadow_caster2D_t, MAX_QUADS);
                }

                shadow_caster2D_t caster = {};
                caster.quad_data = *(g_buffer->quad_buffer + g_buffer->quad_count);

                render_state->draw_frame.shadow_casters[render_state->draw_frame.shadow_caster_counter] = caster; 
                render_state->draw_frame.shadow_caster_counter += 1;
            }
#endif

            g_buffer->quad_count += 1;
        }
    }

    return(result);
}

internal render_quad_t*
r_draw_texture(render_state_t *render_state,
               vec2_t          position,
               vec2_t          render_size,
               vec4_t          color,
               float32         rotation,
               asset_handle_t  texture_handle,
               u32             render_options)
{
    DEBUG_TIMED_BLOCK();
    render_quad_t *result = null;
    
    vec2_t uv_min     = vec2_zero();
    vec2_t uv_max     = vec2_zero();
    if(texture_handle.is_valid)
    {
        uv_min     = *texture_handle.texture->uv_min;
        uv_max     = *texture_handle.texture->uv_max;
    }
    else
    {
        render_options |= RQO_UNTEXTURED;
    }
    result = r_draw_texture_ex(render_state,
                                  position,
                                  render_size,
                                  color,
                                  rotation,
                                  uv_min,
                                  uv_max,
                                  render_options);
    return(result);
}

internal render_quad_t*
r_draw_rect(render_state_t *render_state,
            vec2_t          position,
            vec2_t          render_size,
            vec4_t          color,
            float32         rotation,
            u32             render_options)
{
    DEBUG_TIMED_BLOCK();
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

internal render_line_t* 
r_create_render_line(render_state_t *render_state, 
                     vec2_t          start_point,
                     vec2_t          end_point,
                     float32         thickness,
                     vec4_t          color)
{
    DEBUG_TIMED_BLOCK();
    render_line_t *result = null;

    geometry_buffer_t *g_buffer = r_render_group_get_buffer(render_state, render_state->draw_frame.active_render_group, RGPT_Lines);

    float32 near_value = -1;
    float32 far_value  =  1;

    float32 depth_step        = (far_value - near_value) / MAX_RENDER_LAYERS;
    float32 layer_depth_value = near_value  + (render_state->draw_frame.active_render_layer * depth_step);

    render_line_t new_line = {};
    new_line.layer_depth           = layer_depth_value;
    new_line.start_point.vPosition = vec2_expand_vec3(start_point, layer_depth_value);
    new_line.end_point.vPosition   = vec2_expand_vec3(end_point, layer_depth_value);
    new_line.start_point.vColor    = color;
    new_line.end_point.vColor      = color;

     result = g_buffer->line_buffer + g_buffer->line_count++;
    *result = new_line;

    return(result);
}

//////////////////////////
// STRING RENDERING
//////////////////////////

internal vec2_t 
r_prepare_string_for_rendering(asset_manager_t *asset_manager, dynamic_render_font_varient_t *varient, string_t output)
{
    DEBUG_TIMED_BLOCK();

    vec2_t result = {};
    if(varient != null)
    {
        result = {0, (float32)varient->line_spacing};
        for(u8 *p_character = output.data;
            p_character < output.data + output.count;
            p_character = unicode_next_character(p_character))
        {
            if(*p_character == '\n')
            {
                result.y += varient->line_spacing;
            }

            font_glyph_t *glyph = s_asset_font_get_utf8_glyph(asset_manager, varient, p_character);
            result.x += glyph->glyph_render_size.x + glyph->advance;

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
    }

    return(result);
}

internal vec2_t 
r_draw_string(asset_manager_t *asset_manager,
              render_state_t  *render_state,
              string_t         output,
              asset_handle_t   font,
              u32              pixel_size,
              vec2_t           position,
              vec4_t           color,
              u32              render_options)
{
    DEBUG_TIMED_BLOCK();
    vec2_t result = {};
    dynamic_render_font_varient_t *varient = s_asset_font_get_at_size(asset_manager, font, pixel_size);
    if(varient)
    {
        result = r_prepare_string_for_rendering(asset_manager, varient, output);
        
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
                if(render_state->draw_frame.active_material.texture != &glyph->owner_page->font_atlas)
                {
                    r_renderpass_end(render_state);
                    render_material_t material = r_render_material_create(&glyph->owner_page->font_atlas, 
                                                                           render_state->draw_frame.active_material.shader);

                    r_set_active_render_material(render_state, material);
                    r_renderpass_begin(render_state);
                }

                r_draw_texture_ex(render_state,
                                  vec2(floorf(draw_position.x + glyph->offset_x),
                                       floorf(draw_position.y - glyph->offset_y)),
                                  glyph->glyph_render_size,
                                  color,
                                  0.0f,
                                  glyph->atlas_offset,
                                  glyph->glyph_size,
                                  render_options);

                draw_position.x += glyph->advance;
            }
        }
    }
    else
    {
        log_error("Could not get a varient of your font: '%s' at the size of: '%d'...\n", font.font->filename.data, pixel_size);
    }

    return(result);
}
