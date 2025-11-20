#if !defined(R_DRAW_API_H)
/* ========================================================================
   $File: r_draw_API.h $
   $Date: November 18 2025 02:37 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_DRAW_API_H

/*===========================================
  ======= PIPELINE STATE ADJUSTMENT =========
  ===========================================*/

internal inline void r_set_active_render_material(render_state_t *render_state, render_material_t render_material);
internal inline void r_set_active_render_camera(render_state_t *render_state, render_camera_t *render_camera);
internal inline void r_set_active_render_layer(render_state_t *render_state, u32 layer);
internal inline void r_set_active_render_phase(render_state_t *render_state, u32 render_phase);

internal inline void r_set_active_blend_mode(render_state_t   *render_state, render_group_blending_mode_t src_color_blend, render_group_blending_mode_t dst_color_blend, render_group_blending_mode_t src_alpha_blend, render_group_blending_mode_t dst_alpha_blend);
internal inline void r_set_active_blending_eqs(render_state_t *render_state, render_group_blending_equation_t color_blend_eq, render_group_blending_equation_t alpha_blend_eq);
internal inline void r_set_active_blending_state(render_state_t *render_state, bool32 blending);

internal inline void r_set_active_depth_state(render_state_t *render_state, bool32 depth_test, bool32 depth_mask);
internal inline void r_set_active_depth_func(render_state_t *render_state, render_group_depth_function_t depth_func);

internal void r_pipeline_state_reset(render_state_t *render_state);

/*===========================================
  =========== PRIMITIVE RENDERING ===========
  ===========================================*/
internal render_quad_t  r_create_render_quad(render_state_t *render_state, vec2_t position, vec2_t render_size, vec4_t color, float32 rotation, vec2_t texture_offset, vec2_t texture_size, u32 render_options);
internal render_quad_t* r_draw_texture_ex(render_state_t *render_state, vec2_t position, vec2_t render_size, vec4_t color, float32 rotation, vec2_t texture_offset, vec2_t texture_size, u32 render_options);
internal render_quad_t* r_draw_texture(render_state_t *render_state, vec2_t position, vec2_t render_size, vec4_t color, float32 rotation, asset_handle_t texture_handle, u32 render_options);
internal render_quad_t* r_draw_rect(render_state_t *render_state, vec2_t position, vec2_t render_size, vec4_t color, float32 rotation, u32 render_options);

internal vec2_t 
r_draw_string(asset_manager_t       *asset_manager,
              render_state_t        *render_state,
              string_t               output,
              asset_handle_t         font,
              u32                    pixel_size,
              vec2_t                 position,
              vec4_t                 color,
              u32                    render_options);


#endif // R_DRAW_API_H

