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

    global_game_state.game_camera.view_matrix       = view_matrix;
    global_game_state.game_camera.projection_matrix = projection_matrix;

    asset_handle_t player_texture = s_asset_texture_get(asset_manager, STR("player"));

    render_material_t material = r_render_material_create(&player_texture.asset_slot->texture, &render_state->test_shader);
    r_pipeline_state_reset(render_state);

    r_set_active_render_phase(render_state, RGP_Preblit);
    r_set_active_render_camera(render_state, &global_game_state.game_camera);
    r_set_active_render_material(render_state, material);
    r_set_active_render_layer(render_state, 31);
    r_set_active_depth_state(render_state, true, true);
    r_set_active_blending_state(render_state, true);

    r_renderpass_begin(render_state);
    r_draw_texture(render_state, {0, 0}, {10, 10}, COLOR_WHITE, 0, player_texture, RQO_NONE);
    r_renderpass_end(render_state);

    r_pipeline_state_reset(render_state);
}
