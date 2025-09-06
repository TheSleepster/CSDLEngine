/* ========================================================================
   $File: g_main.c $
   $Date: Wed, 30 Jul 25: 05:28PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define COLOR_WHITE  ((vec4_t){1.0, 1.0, 1.0, 1.0})
#define COLOR_RED    ((vec4_t){1.0, 0.0, 0.0, 1.0})
#define COLOR_GREEN  ((vec4_t){0.0, 1.0, 0.0, 1.0})
#define COLOR_BLUE   ((vec4_t){0.0, 0.0, 1.0, 1.0})
#define COLOR_BLACK  ((vec4_t){0.0, 0.0, 0.0, 1.0})

global bool8 initialized_stuff;
global dynamic_render_font_varient_t *varient;

internal void
r_DEBUG_test_render(render_state_t *render_state, audio_manager_t *audio_manager, asset_manager_t *asset_manager, float32 delta_time)
{
    draw_frame_t *draw_frame = &render_state->draw_frame;

    mat4_t projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1);
    mat4_t view_matrix       = mat4_identity();

    render_group_desc_t test_group_desc = r_build_renderpass_desc(&render_state->test_shader,
                                                                  16,
                                                                  view_matrix,
                                                                  projection_matrix,
                                                                  RGE_None);
    r_begin_renderpass(render_state, &test_group_desc);
    r_update_shader_uniform_data(&render_state->test_shader, STR("uProjectionMatrix"), &draw_frame->active_render_group->render_desc.projection_matrix.values);
    r_update_shader_uniform_data(&render_state->test_shader, STR("uViewMatrix"),       &draw_frame->active_render_group->render_desc.view_matrix.values);
    r_update_shader_uniform_data(&render_state->test_shader, STR("uEffectMask"),       &draw_frame->active_render_group->render_desc.desired_effects);

    r_draw_rect(render_state, (vec2_t){ 0,   0},  (vec2_t){16, 16}, (vec4_t){1, 0, 0, 1}, 45,   RQO_NONE);
    r_draw_rect(render_state, (vec2_t){ 20,  0},  (vec2_t){16, 16}, (vec4_t){1, 0, 1, 1}, 20,   RQO_NONE);
    r_draw_rect(render_state, (vec2_t){ 40,  0},  (vec2_t){16, 16}, (vec4_t){0, 0, 1, 1}, 15,   RQO_NONE);
    r_draw_rect(render_state, (vec2_t){-20,  0},  (vec2_t){16, 16}, (vec4_t){0, 1, 0, 1}, 10,   RQO_NONE);
    r_end_renderpass(render_state);

    render_group_desc_t test_group2 = test_group_desc;
    test_group2.render_layer        = 16;

    r_begin_renderpass(render_state, &test_group2);
    r_draw_rect(render_state, (vec2_t){0,  20}, (vec2_t){16, 16}, (vec4_t){1, 1, 1, 1}, 33, RQO_NONE);
    r_end_renderpass(render_state);

    render_group_desc_t test_group3 = test_group_desc;
    test_group3.render_layer    = 17;
    test_group3.desired_effects = RGE_Lighting;

    r_begin_renderpass(render_state, &test_group3);
    r_draw_rect(render_state, (vec2_t){0, -20}, (vec2_t){16, 16}, (vec4_t){0, 1, 1, 1}, 10, RQO_SHADOWCASTER);

    // asset_handle_t lm_font_handle     = s_asset_font_get(asset_manager, STR("LiberationMono_Regular"));
    asset_handle_t arial_font_handle = s_asset_font_get(asset_manager, STR("arial"));
    // asset_handle_t atari_font_handle = s_asset_font_get(asset_manager, STR("AtariClassic_gry3"));

    asset_handle_t block_handle      = s_asset_texture_get(asset_manager, STR("block"));
    //asset_handle_t test_handle       = s_asset_loaded_sound_get(asset_manager, STR("Test2"));
    if(!initialized_stuff || audio_manager->first_playing_sound == null)
    {
        if(audio_manager->playing_sound_arena.is_initialized == false)
        {
            audio_manager->playing_sound_arena = c_arena_create(MB(100));
        }
        //s_asset_playing_sound_create(audio_manager, test_handle, vec2_create_float(1.0f, 1.0f));

        initialized_stuff = true;
    }

    r_draw_texture(render_state, vec2_create_float(20, 40),     vec2_create_float(16, 16), COLOR_WHITE, 0, block_handle, RQO_NONE);
    r_draw_rect(render_state,    vec2_create_float(-50.0, -40), vec2_create_float(30, 30), COLOR_WHITE, 0, RQO_NONE);
    r_draw_texture(render_state, vec2_create_float(50, -50),    vec2_create_float(16, 16), COLOR_WHITE, 0, block_handle, RQO_NONE);
    
    r_end_renderpass(render_state);

    mat4_t font_projection_matrix = mat4_RHGL_ortho(-960, 960, -540, 540, -1, 1);
    mat4_t font_view_matrix       = mat4_identity();
    render_group_desc_t test_group4 = r_build_renderpass_desc(&render_state->font_shader,
                                                              15,
                                                              font_view_matrix,
                                                              font_projection_matrix,
                                                              RGE_None,
                                                              RGP_PostBlitPass);
    r_begin_renderpass(render_state, &test_group4);

    r_update_shader_uniform_data(&render_state->font_shader, STR("uProjectionMatrix"), &draw_frame->active_render_group->render_desc.projection_matrix.values);
    r_update_shader_uniform_data(&render_state->font_shader, STR("uViewMatrix"),       &draw_frame->active_render_group->render_desc.view_matrix.values);

    DEBUG_display_records(asset_manager, render_state, arial_font_handle, delta_time);

    r_end_renderpass(render_state);

    render_group_desc_t test_group5 = r_build_renderpass_desc(&render_state->test_shader,
                                                              2,
                                                              view_matrix,
                                                              projection_matrix,
                                                              RGE_Lighting,
                                                              RGP_MainGamePass,
                                                              RGPT_Quads,
                                                              true);
    r_begin_renderpass(render_state, &test_group5);
    r_draw_rect(render_state, vec2_create_float(10, 40), vec2_create_float(20, 20), vec4_create_float4(1.0f, 0.0f, 0.0f, 0.05f), 0, RQO_NONE);
    r_draw_rect(render_state, vec2_create_float(10, 40), vec2_create_float(20, 20), vec4_create_float4(1.0f, 0.0f, 0.0f, 0.05f), 0, RQO_NONE);
    r_end_renderpass(render_state);

    render_group_desc_t test_group6 = r_build_renderpass_desc(&render_state->test_shader,
                                                              2,
                                                              view_matrix,
                                                              projection_matrix,
                                                              RGE_None,
                                                              RGP_PostBlitPass,
                                                              RGPT_Lines,
                                                              false);
    r_begin_renderpass(render_state, &test_group6);
    r_create_render_line(render_state, vec2_create_float(-100.0, -70), vec2_create_float(100, -55), 1.0f, COLOR_WHITE);
    r_end_renderpass(render_state);

    render_group_desc_t test_group7 = r_build_renderpass_desc(&render_state->test_shader,
                                                              31,
                                                              view_matrix,
                                                              projection_matrix,
                                                              RGE_None,
                                                              RGP_MainGamePass,
                                                              RGPT_Quads,
                                                              false);
    r_begin_renderpass(render_state, &test_group7);
    r_draw_rect(render_state, vec2_create_float(-160, 90), vec2_create_float(320, -180), COLOR_BLACK, 0, RQO_NONE);
    r_end_renderpass(render_state);
}

internal void
initialize_gamestate()
{
}

internal void
g_update_and_render(render_state_t *render_state, audio_manager_t *audio_manager, asset_manager_t *asset_manager, float32 delta_time)
{
    r_DEBUG_test_render(render_state, audio_manager, asset_manager, delta_time);
}
