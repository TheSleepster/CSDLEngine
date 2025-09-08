#if !defined(R_RENDER_API_H)
/* ========================================================================
   $File: r_render_API.h $
   $Date: Sat, 06 Sep 25: 04:13PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_RENDER_API_H
#include "r_renderer_data.h"

internal void                       r_add_texture_to_texture_list(render_group_t *group, u32 textureID);
internal render_quad_t              r_create_render_quad(render_state_t *render_state, vec2_t position, vec2_t render_size, vec4_t color, float32 rotation, vec2_t texture_offset, vec2_t texture_size, u32 gpu_texture_id, render_quad_options_t render_options);
internal render_quad_t*             r_draw_texture_ex(render_state_t *render_state, vec2_t position, vec2_t render_size, vec4_t color, float32 rotation, vec2_t texture_offset, vec2_t texture_size, u32 gpu_texture_id, render_quad_options_t render_options);
internal inline render_quad_t*      r_draw_texture(render_state_t *render_state, vec2_t position, vec2_t render_size, vec4_t color, float32 rotation, asset_handle_t texture_handle, render_quad_options_t render_options);
internal render_quad_t*             r_draw_rect(render_state_t *render_state, vec2_t position, vec2_t render_size, vec4_t color, float32 rotation, render_quad_options_t render_options);
internal s32                        r_prepare_string_for_rendering(asset_manager_t *asset_manager, dynamic_render_font_varient_t *varient, string_t output);
internal void                       r_draw_string(asset_manager_t *asset_manager, render_state_t *render_state, string_t output, asset_handle_t font, u32 pixel_size, vec2_t position, vec4_t color, render_quad_options_t render_options);
internal render_line_t*             r_create_render_line(render_state_t *render_state, vec2_t start_point, vec2_t end_point, float32 thickness, vec4_t color);
internal point_light_t*             r_create_point_light(render_state_t *render_state, vec2_t position, vec4_t color, float32 radius);
internal void                       r_handle_lighting_data(render_state_t *render_state);
internal inline render_group_desc_t r_build_renderpass_desc(GPU_shader_t *desired_shader, u32 render_layer, mat4_t view_matrix, mat4_t projection_matrix, render_group_effects_t render_effects, render_group_desired_render_phase render_phase, render_group_primitive_type_t primitive_type, bool8 supports_transparency);
internal u64                        r_get_renderpass_desc_id(render_group_desc_t *render_pass_desc);
internal void                       r_begin_renderpass(render_state_t *render_state, render_group_desc_t *render_pass_desc);
internal inline void                r_end_renderpass(render_state_t *render_state);
internal void                       r_fill_render_group_vertex_buffer(render_group_t *render_group);
internal void                       r_handle_renderpass_data(asset_manager_t *asset_manager, render_state_t *render_state);

internal void                       DEBUG_display_record_data(asset_manager_t *asset_manager, render_state_t *render_state, asset_handle_t font, float32 delta_time);
//internal void                       DEBUG_display_records(asset_manager_t *asset_manager, render_state_t *render_state, asset_handle_t font, float32 delta_time);
#endif
