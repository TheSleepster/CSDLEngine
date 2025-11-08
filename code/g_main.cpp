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

constexpr float32 GRAVITY_A    = 1.5f;
constexpr u32     MAX_ENTITIES = 10000;

constexpr float32 UPDATE_RATE  = 1.0f / 60.0f; 

global float32 accumulator = 0.0f;

enum entity_type
{
    ET_Player,
    ET_Count
};

enum entity_flags
{
    EF_Valid    = 1 << 0,
    EF_Jumping  = 1 << 1,
    EF_Falling  = 1 << 2,
    EF_Running  = 1 << 3,
    EF_Alive    = 1 << 4,
    EF_Gravitic = 1 << 5,
    EF_Actor    = 1 << 6,
    EF_Count
};

struct sprite_t
{
    asset_handle_t texture;
    vec2_t         size;
    u32            frame_count;

    string_t       name;
};

struct game_clock_t
{
    float32 start_time;
    float32 end_time;

    float32 elapsed;
    float32 duration;
};

struct entity_t
{
    u64      e_flags;
    u32      e_type;
    u32      render_options;

    sprite_t sprite;

    vec2_t   max_vel;
    float32  max_accel;

    vec2_t   position;
    vec2_t   velocity;
    vec4_t   render_color;

    vec2_t   render_size;
    float32  rotation;

    bool8    is_on_ground;
};

struct entity_manager_t
{
    entity_t entities[MAX_ENTITIES];
    u32      active_entity_count;
};

struct game_state_t 
{
    bool8  is_initialized;

    input_controller_t *controller;
    render_group_t     *entity_render_group;
    entity_manager_t    entity_manager;

    entity_t           *player;
    vec2_t              input_axis;

    file_t              input_data_file;
    bool8               recording_input;
    bool8               replaying_input;
};

global game_state_t global_game_state;

internal entity_t* 
entity_create(entity_manager_t *entity_manager)
{
    entity_t *new_entity = null;
    for(u32 entity_index = 1;
        entity_index < MAX_ENTITIES;
        ++entity_index)
    {
        entity_t *entity = entity_manager->entities + entity_index;
        if(!(entity->e_flags & EF_Valid))
        {
            new_entity = entity;
            break;
        }
    }
    Assert(new_entity);

    entity_manager->active_entity_count++;
    new_entity->e_flags = EF_Valid;

    return(new_entity);
}

internal sprite_t
entity_init_sprite_data(asset_manager_t *asset_manager, string_t sprite_name)
{
    sprite_t result = {};

    result.texture = s_asset_texture_get(asset_manager, sprite_name);
    result.name = sprite_name;
    if(result.texture.is_valid)
    {
        result.size = vec2(result.texture.asset_slot->texture.bitmap.width, result.texture.asset_slot->texture.bitmap.height);
    }
    else
    {
        log_error("Sprite name '%s' is invalid... Try rebuilding the asset file or verify the name is correct...\n", 
                  C_STR(sprite_name));
    }

    return(result);
}

internal entity_t*
entity_player_create(entity_manager_t *entity_manager, asset_manager_t *asset_manager, vec2_t position)
{
    entity_t *player  = entity_create(entity_manager);
    ZeroStruct(*player);

    player->e_type       = ET_Player;
    player->e_flags     |= (EF_Alive|EF_Gravitic|EF_Actor);
    player->sprite       = entity_init_sprite_data(asset_manager, STR("player"));
    player->render_size  = player->sprite.size;
    player->render_size  = vec2(10, 10);
    player->render_color = COLOR_WHITE;
    player->position     = position;
    player->max_accel    = 2.5f;
    player->max_vel      = vec2(5.0f, 5.0f);

    return(player);
}

internal void
entity_render(render_state_t *render_state, asset_manager_t *asset_manager, entity_t *entity)
{
    entity->sprite = entity_init_sprite_data(asset_manager, entity->sprite.name);
    r_draw_texture(render_state, 
                   entity->position, 
                   entity->render_size, 
                   entity->render_color, 
                   entity->rotation, 
                   entity->sprite.texture, 
                   (render_quad_options_t)entity->render_options);
}

internal void
initialize_gamestate(render_state_t *render_state, input_manager_t *input_manager, asset_manager_t *asset_manager)
{
    r_reset_draw_frame_pipeline_state(render_state);
    global_game_state.controller = s_input_manager_get_primary_controller(input_manager);
    global_game_state.input_data_file = c_file_open(STR("InputData.idf"), true);

    mat4_t projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1);
    mat4_t view_matrix       = mat4_identity();
    render_group_desc_t test_group_desc = r_renderpass_build_pass_desc(render_state,
                                                                      &render_state->test_shader,
                                                                       16,
                                                                       view_matrix,
                                                                       projection_matrix,
                                                                       RGE_None);
    global_game_state.entity_render_group = r_renderpass_get_or_create(render_state, &test_group_desc);
    r_renderpass_end(render_state);
    log_info("Input Manager is size: '%d'\n", sizeof(input_manager_t));

    global_game_state.entity_manager.active_entity_count = 0;
    global_game_state.player = entity_player_create(&global_game_state.entity_manager, asset_manager, vec2_zero());
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
    if(frame_time > (UPDATE_RATE * 2.0f))
    {
        frame_time = UPDATE_RATE * 2.0f;
    }
    
    if(!global_game_state.is_initialized) 
    {
        initialize_gamestate(render_state, input_manager, asset_manager);
        global_game_state.is_initialized = true;
    }

    global_game_state.input_axis = {};
    if(s_input_manager_is_keyboard_key_down(global_game_state.controller, SDL_SCANCODE_W))
    { 
        global_game_state.input_axis.y = 1.0f;
    }
    if(s_input_manager_is_keyboard_key_down(global_game_state.controller, SDL_SCANCODE_A))
    {
        global_game_state.input_axis.x = -1.0f;
    }
    if(s_input_manager_is_keyboard_key_down(global_game_state.controller, SDL_SCANCODE_S))
    {
        global_game_state.input_axis.y = -1.0f;
    }
    if(s_input_manager_is_keyboard_key_down(global_game_state.controller, SDL_SCANCODE_D))
    {
        global_game_state.input_axis.x =  1.0f;
    }
    global_game_state.input_axis = vec2_normalize(global_game_state.input_axis);

    // NOTE(Sleepster): entity update loop
    for(u32 entity_index = 0;
        entity_index <= global_game_state.entity_manager.active_entity_count;
        ++entity_index)
    {
        entity_t *entity = global_game_state.entity_manager.entities + entity_index;
        if((entity->e_flags & EF_Actor) != 0)
        {
            if(entity->e_type == ET_Player)
            {
                if(entity->is_on_ground)
                {
                    entity->velocity.x += global_game_state.input_axis.x * (10.0f * frame_time);

                    if(s_input_manager_is_keyboard_key_pressed(global_game_state.controller, SDL_SCANCODE_SPACE))
                    {
                        entity->is_on_ground = false;
                        entity->velocity.y   = 300.0f * frame_time; 
                    }
                }
                entity->velocity.y += -10.0f * frame_time;
                entity->position = vec2_add(entity->position, entity->velocity);

                if(entity->position.y <= 0.0)
                {
                    entity->is_on_ground = true;
                    entity->velocity.y   = 0.0f;
                    entity->position.y   = 0.0f;
                }

                entity->velocity = vec2_scale(entity->velocity, 0.95f);
            }
        }
    }

    // NOTE(Sleepster): entity render loop 
    r_renderpass_begin(render_state, global_game_state.entity_render_group);
    for(u32 entity_index = 0;
        entity_index <= global_game_state.entity_manager.active_entity_count;
        ++entity_index)
    {
        entity_t *entity = global_game_state.entity_manager.entities + entity_index;
        entity_render(render_state, asset_manager, entity);
    }
    r_renderpass_end(render_state);
}

