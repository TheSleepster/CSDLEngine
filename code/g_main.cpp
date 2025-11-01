/* ========================================================================
   $File: g_main.c $
   $Date: Wed, 30 Jul 25: 05:28PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "l_runtime_data.cpp"
#include "g_test.cpp"

#define COLOR_WHITE  ((vec4_t){1.0, 1.0, 1.0, 1.0})
#define COLOR_RED    ((vec4_t){1.0, 0.0, 0.0, 1.0})
#define COLOR_GREEN  ((vec4_t){0.0, 1.0, 0.0, 1.0})
#define COLOR_BLUE   ((vec4_t){0.0, 0.0, 1.0, 1.0})
#define COLOR_BLACK  ((vec4_t){0.0, 0.0, 0.0, 1.0})

struct game_state_t 
{
    bool8  is_initialized;
    vec2_t player_pos;

    input_controller_t *controller;
    render_group_t     *entity_group;

    file_t              input_data_file;
    bool8               recording_input;
    bool8               replaying_input;
};

global game_state_t global_game_state;

internal void
initialize_gamestate(render_state_t *render_state, input_manager_t *input_manager)
{
    r_reset_draw_frame_pipeline_state(render_state);
    global_game_state.controller = s_input_manager_get_primary_controller(input_manager);
    global_game_state.input_data_file = c_file_open(STR("InputData.idf"), true);

    mat4_t projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1);
    mat4_t view_matrix       = mat4_identity();
    render_group_desc_t test_group_desc = r_build_renderpass_desc(render_state,
                                                                  &render_state->test_shader,
                                                                  16,
                                                                  view_matrix,
                                                                  projection_matrix,
                                                                  RGE_None);
    global_game_state.entity_group = r_begin_renderpass(render_state, &test_group_desc);
    r_end_renderpass(render_state);
    log_info("Input Manager is size: '%d'\n", sizeof(input_manager_t));
}

GAME_API external
GAME_UPDATE_AND_RENDER(g_update_and_render)
{
#if DEVELOPER_BUILD
    if(global_context == null)
    {
        global_context = context;
        DEBUG_global_state = DEBUG_global_state_in;
    }
#endif
#ifdef INTERNAL_DEBUG
    DEBUG_TIMED_BLOCK();
#endif
    //r_DEBUG_test_render(render_state, audio_manager, asset_manager, delta_time);
    if(!global_game_state.is_initialized) 
    {
        initialize_gamestate(render_state, input_manager);
        global_game_state.is_initialized = true;
    }

    vec2_t input_axis = {};
    if(s_input_manager_is_keyboard_key_down(global_game_state.controller, SDL_SCANCODE_W))
    { 
        input_axis.y = 1.0f;
    }
    if(s_input_manager_is_keyboard_key_down(global_game_state.controller, SDL_SCANCODE_A))
    {
        input_axis.x = -1.0f;
    }
    if(s_input_manager_is_keyboard_key_down(global_game_state.controller, SDL_SCANCODE_S))
    {
        input_axis.y = -1.0f;
    }
    if(s_input_manager_is_keyboard_key_down(global_game_state.controller, SDL_SCANCODE_D))
    {
        input_axis.x =  1.0f;
    }
    input_axis = vec2_normalize(input_axis);

    float32 speed = 250.0f;
    global_game_state.player_pos = vec2_add(global_game_state.player_pos, vec2(input_axis.x * (speed * delta_time), input_axis.y * (speed * delta_time)));

    r_begin_renderpass(render_state, global_game_state.entity_group);
    r_draw_rect(render_state, global_game_state.player_pos, {10, 10}, COLOR_WHITE, 0.0, RQO_NONE);
    r_end_renderpass(render_state);
}
