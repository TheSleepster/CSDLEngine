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
    
    r_reset_draw_frame_pipeline_state(render_state);
    mat4_t projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1);
    mat4_t view_matrix       = mat4_identity();

    render_group_desc_t test_group_desc = r_renderpass_build_pass_desc(render_state,
                                                                      &render_state->test_shader,
                                                                       16,
                                                                       view_matrix,
                                                                       projection_matrix,
                                                                       RGE_None);
    r_renderpass_get_or_create(render_state, &test_group_desc);
    r_draw_rect(render_state, (vec2_t){ 0,   0},  (vec2_t){16, 16}, (vec4_t){1, 0, 0, 1}, 45,   RQO_NONE);
    r_draw_rect(render_state, (vec2_t){ 20,  0},  (vec2_t){16, 16}, (vec4_t){1, 0, 1, 1}, 20,   RQO_NONE);
    r_draw_rect(render_state, (vec2_t){ 40,  0},  (vec2_t){16, 16}, (vec4_t){0, 0, 1, 1}, 15,   RQO_NONE);
    r_draw_rect(render_state, (vec2_t){-20,  0},  (vec2_t){16, 16}, (vec4_t){0, 1, 0, 1}, 10,   RQO_NONE);
    r_renderpass_end(render_state);

    render_group_desc_t test_group2 = test_group_desc;
    test_group2.render_layer        = 16;

    r_renderpass_get_or_create(render_state, &test_group2);
    r_draw_rect(render_state, (vec2_t){0,  20}, (vec2_t){16, 16}, (vec4_t){1, 1, 1, 1}, 33, RQO_NONE);
    r_renderpass_end(render_state);

    render_group_desc_t test_group3 = test_group_desc;
    test_group3.render_layer    = 17;
    test_group3.desired_effects = RGE_Lighting;

    r_renderpass_get_or_create(render_state, &test_group3);
    r_draw_rect(render_state, (vec2_t){0, -20}, (vec2_t){16, 16}, (vec4_t){0, 1, 1, 1}, 10, RQO_SHADOWCASTER);

    asset_handle_t lm_font_handle    = s_asset_font_get(asset_manager, STR("LiberationMono_Regular"));
    //asset_handle_t test_handle       = s_asset_loaded_sound_get(asset_manager, STR("Test2"));
    //asset_handle_t atari_font_handle = s_asset_font_get(asset_manager, STR("AtariClassic_gry3"));

    //asset_handle_t arial_font_handle = s_asset_font_get(asset_manager, STR("arial"));
    asset_handle_t block_handle      = s_asset_texture_get(asset_manager, STR("block"));
    if(!initialized_stuff || audio_manager->first_playing_sound == null)
    {
        if(audio_manager->playing_sound_arena.is_initialized == false)
        {
            audio_manager->playing_sound_arena = c_arena_create(MB(100));
        }
        //s_asset_playing_sound_create(audio_manager, test_handle, vec2(1.0f, 1.0f));

        initialized_stuff = true;
    }

    r_draw_texture(render_state, vec2(20, 40),  vec2(16, 16), COLOR_WHITE, 0, block_handle, RQO_NONE);
    //r_draw_rect(render_state,    vec2(-50.0, -40), vec2(30, 30), COLOR_WHITE, 0, RQO_NONE);
    r_draw_texture(render_state, vec2(50, -50), vec2(16, 16), COLOR_WHITE, 0, block_handle, RQO_NONE);
    
    r_renderpass_end(render_state);

    mat4_t font_projection_matrix   = mat4_RHGL_ortho(-960, 960, -540, 540, -1, 1);
    mat4_t font_view_matrix         = mat4_identity();
    render_group_desc_t test_group4 = r_renderpass_build_pass_desc(render_state,
                                                                   &render_state->font_shader,
                                                                   15,
                                                                   font_view_matrix,
                                                                   font_projection_matrix,
                                                                   RGE_None,
                                                                   RGP_PostBlitPass);
    r_renderpass_get_or_create(render_state, &test_group4);

    r_draw_string(asset_manager,
                  render_state,
                  STR("This is another test of the rendering engine...\nDoes this font render properly?\nPerhaps there's an issue we don't know about?\nThe quick brown fox jumps over the lazy dog\nTHE QUICK BROWN FOX JUMPS OVER THE WIRED FENCE"),
                  lm_font_handle,
                  16,
                  vec2(-800, -300),
                  COLOR_WHITE,
                  RQO_NONE);
    
    r_renderpass_end(render_state);

    r_set_active_blending_state(render_state, true);
    r_set_active_depth_state(render_state, true, false);

    r_set_active_blend_mode(render_state, RGBM_One, RGBM_OneMinusSrcColor, RGBM_One, RGBM_OneMinusSrcAlpha);
    render_group_desc_t test_group5 = r_renderpass_build_pass_desc(render_state,
                                                                  &render_state->test_shader,
                                                                   14,
                                                                   view_matrix,
                                                                   projection_matrix,
                                                                   RGE_Lighting,
                                                                   RGP_MainGamePass,
                                                                   RGPT_Quads);
    r_renderpass_get_or_create(render_state, &test_group5);
    r_draw_rect(render_state, vec2(10, 40), vec2(20, 20), vec4(1.0f, 0.0f, 0.0f, 0.05f), 0, RQO_NONE);
    r_renderpass_end(render_state);

    r_set_active_blending_state(render_state, false);
    r_set_active_depth_state(render_state, true, true);
    render_group_desc_t test_group6 = r_renderpass_build_pass_desc(render_state,
                                                                  &render_state->test_shader,
                                                                   14,
                                                                   view_matrix,
                                                                   projection_matrix,
                                                                   RGE_None,
                                                                   RGP_PostBlitPass,
                                                                   RGPT_Lines);
    r_renderpass_get_or_create(render_state, &test_group6);
    r_create_render_line(render_state, vec2(-100.0, -70), vec2(100, -70), 1.0f, COLOR_WHITE);
    r_renderpass_end(render_state);

    render_group_desc_t test_group7 = r_renderpass_build_pass_desc(render_state,
                                                                  &render_state->test_shader,
                                                                   31,
                                                                   view_matrix,
                                                                   projection_matrix,
                                                                   RGE_None,
                                                                   RGP_MainGamePass,
                                                                   RGPT_Quads);
    r_renderpass_get_or_create(render_state, &test_group7);
    r_draw_rect(render_state, vec2(-160, 90), vec2(320, -180), {0.3, 0.3, 0.3, 1.0}, 0, RQO_NONE);
    r_renderpass_end(render_state);

    r_reset_draw_frame_pipeline_state(render_state);
}

