/* ========================================================================
   $File: g_main.c $
   $Date: Wed, 30 Jul 25: 05:28PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
internal void
r_DEBUG_test_render(render_state_t *render_state)
{
    mat4_t projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1);
    mat4_t view_matrix       = mat4_identity();

    render_group_desc_t test_group_desc = r_build_renderpass_desc(&render_state->test_shader,
                                                                  16,
                                                                  view_matrix,
                                                                  projection_matrix,
                                                                  RGE_None);
    r_begin_renderpass(render_state, &test_group_desc);
    r_draw_quad(render_state, (vec2_t){ 0,   0},  (vec2_t){20, 20}, (vec4_t){1, 0, 0, 1}, 45,   RQO_NONE);
    r_draw_quad(render_state, (vec2_t){ 20,  0},  (vec2_t){20, 20}, (vec4_t){1, 0, 1, 1}, 20,   RQO_NONE);
    r_draw_quad(render_state, (vec2_t){ 40,  0},  (vec2_t){20, 20}, (vec4_t){0, 0, 1, 1}, 15,   RQO_NONE);
    r_draw_quad(render_state, (vec2_t){-20,  0},  (vec2_t){20, 20}, (vec4_t){0, 1, 0, 1}, 10,   RQO_NONE);
    r_end_renderpass(render_state);

    render_group_desc_t test_group2 = test_group_desc;
    test_group2.render_layer   = 16;

    r_begin_renderpass(render_state, &test_group2);
    r_draw_quad(render_state, (vec2_t){0,  20}, (vec2_t){20, 20}, (vec4_t){1, 1, 1, 1}, 33, RQO_NONE);
    r_end_renderpass(render_state);

    render_group_desc_t test_group3 = test_group_desc;
    test_group3.render_layer   = 17;
    test_group3.render_effects = RGE_Lighting;

    r_begin_renderpass(render_state, &test_group3);
    r_draw_quad(render_state, (vec2_t){0, -20}, (vec2_t){20, 20}, (vec4_t){0, 1, 1, 1}, 10, RQO_SHADOWCASTER);
    r_end_renderpass(render_state);
}

internal void
g_setup()
{
}

internal void
g_update_and_render(bool8 game_initialized, render_state_t *render_state)
{
    if(!game_initialized)
    {
        g_setup();
        game_initialized = true;
    }

    r_DEBUG_test_render(render_state);
}
