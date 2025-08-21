/* ========================================================================
   $File: g_main.c $
   $Date: Wed, 30 Jul 25: 05:28PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define COLOR_WHITE ((vec4_t){1.0, 1.0, 1.0, 1.0})

global bool8 initialized_stuff;
global dynamic_render_font_varient_t *varient;

internal void
r_DEBUG_test_render(render_state_t *render_state, asset_manager_t *asset_manager)
{
    mat4_t projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1);
    mat4_t view_matrix       = mat4_identity();

    render_group_desc_t test_group_desc = r_build_renderpass_desc(&render_state->test_shader,
                                                                  16,
                                                                  view_matrix,
                                                                  projection_matrix,
                                                                  RGE_None);
    r_begin_renderpass(render_state, &test_group_desc);
    r_draw_rect(render_state, (vec2_t){ 0,   0},  (vec2_t){20, 20}, (vec4_t){1, 0, 0, 1}, 45,   RQO_NONE);
    r_draw_rect(render_state, (vec2_t){ 20,  0},  (vec2_t){20, 20}, (vec4_t){1, 0, 1, 1}, 20,   RQO_NONE);
    r_draw_rect(render_state, (vec2_t){ 40,  0},  (vec2_t){20, 20}, (vec4_t){0, 0, 1, 1}, 15,   RQO_NONE);
    r_draw_rect(render_state, (vec2_t){-20,  0},  (vec2_t){20, 20}, (vec4_t){0, 1, 0, 1}, 10,   RQO_NONE);
    r_end_renderpass(render_state);

    render_group_desc_t test_group2 = test_group_desc;
    test_group2.render_layer   = 16;

    r_begin_renderpass(render_state, &test_group2);
    r_draw_rect(render_state, (vec2_t){0,  20}, (vec2_t){20, 20}, (vec4_t){1, 1, 1, 1}, 33, RQO_NONE);
    r_end_renderpass(render_state);

    render_group_desc_t test_group3 = test_group_desc;
    test_group3.render_layer   = 17;
    test_group3.render_effects = RGE_Lighting;

    r_begin_renderpass(render_state, &test_group3);
    r_draw_rect(render_state, (vec2_t){0, -20}, (vec2_t){20, 20}, (vec4_t){0, 1, 1, 1}, 10, RQO_SHADOWCASTER);

    asset_handle_t font_handle  = s_asset_font_get(asset_manager, STR("LiberationMono_Regular"));
    asset_handle_t block_handle = s_asset_texture_get(asset_manager, STR("block"));
    if(!initialized_stuff)
    {
        s_asset_font_create_at_size(asset_manager, font_handle, 14);
        varient = s_asset_font_get_at_size(asset_manager, font_handle, 16);
        s_asset_texture_load_data(asset_manager, block_handle);
        
        initialized_stuff = true;
    }

    r_draw_texture(render_state, vec2_create_float(20, 40), vec2_create_float(20, 20), COLOR_WHITE, 0, block_handle, RQO_NONE);

    r_draw_rect(render_state, vec2_create_float(-50.0, -40), vec2_create_float(30, 30), COLOR_WHITE, 0, RQO_NONE);
    r_draw_texture(render_state, vec2_create_float(50, -50), vec2_create_float(20, 20), COLOR_WHITE, 0, block_handle, RQO_NONE);
    
    r_end_renderpass(render_state);
}

internal void
initialize_gamestate()
{
}

global bool8 initialized_stuff = false;

internal void
g_update_and_render(render_state_t *render_state, asset_manager_t *asset_manager)
{
    r_DEBUG_test_render(render_state, asset_manager);
}
