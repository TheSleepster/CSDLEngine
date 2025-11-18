/* ========================================================================
   $File: g_test.cpp $
   $Date: October 28 2025 12:20 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
global bool8 initialized_stuff;

internal void
r_DEBUG_test_render(render_state_t *render_state, audio_manager_t *audio_manager, asset_manager_t *asset_manager, float32 delta_time)
{
    DEBUG_TIMED_BLOCK();

    mat4_t projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1);
    mat4_t view_matrix       = mat4_identity();

    render_camera_t camera;
    camera.view_matrix = view_matrix;
    camera.projection_matrix = projection_matrix;

    render_material_t material = {};
    material.shader = &render_state->test_shader;

    r_pipeline_state_reset(render_state);
    r_set_active_render_camera(render_state, camera);
    r_set_active_render_material(render_state, material);
    r_set_active_render_layer(render_state, 16);
    r_set_active_render_phase(render_state, RGP_Preblit);

    r_renderpass_begin(render_state);
    r_draw_rect(render_state, {0, 0}, {20, 20}, COLOR_RED, 0, RQO_NONE);
    r_renderpass_end(render_state);
}
