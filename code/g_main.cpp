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

constexpr float32 GRAVITY_A      = 1.5f;
constexpr float32 UPDATE_RATE    = 1.0f / 60.0f; 
constexpr u32     MAX_ENTITIES   = 10000;

global float32 dt_accumulator = 0.0f;

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

struct event_clock_t
{
    float32 start_time;
    float32 end_time;

    float32 elapsed;
    float32 duration;
};

struct entity_t
{
    u64           e_flags;
    u32           e_type;
    u32           render_options;

    sprite_t      sprite;
    s32           facing_dir;

    float32       x_accel;
    float32       movement_speed;
    float32       terminal_velocity;
    float32       jump_speed;
    float32       dash_speed;
    float32       gravity_intensity;
    float32       friction_scale;
    vec2_t        velocity;

    event_clock_t dash_timer;

    vec2_t        position;
    vec2_t        prev_position;
    vec4_t        render_color;
    vec2_t        render_size;
    float32       rotation;

    bool8         is_on_ground;
};

struct entity_manager_t
{
    entity_t entities[MAX_ENTITIES];
    u32      active_entity_count;
};


struct game_state_t 
{
    bool8  is_initialized;
    u32    play_state;

    input_controller_t *controller;
    render_group_t     *entity_render_group;
    entity_manager_t    entity_manager;

    entity_t           *player;
    vec2_t              input_axis;
    bool8               should_jump;
    bool8               should_dash;

    file_t              input_data_file;
    bool8               recording_input;
    bool8               replaying_input;

    bool8               in_editor;
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

    // TODO(Sleepster): Fix having to do this every time. 
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
    entity_t *player = entity_create(entity_manager);
    ZeroStruct(*player);

    player->e_type              = ET_Player;
    player->e_flags            |= (EF_Alive|EF_Gravitic|EF_Actor);
    player->sprite              = entity_init_sprite_data(asset_manager, STR("player"));
    player->render_size         = player->sprite.size;
    player->render_size         = vec2(10, 10);
    player->render_color        = COLOR_WHITE;
    player->position            = position;

    player->x_accel             =  20.0f;
    player->movement_speed      =  2.0f;
    player->terminal_velocity   =  5.0f; 
    player->jump_speed          =  300.0f;
    player->dash_speed          =  300.0f;
    player->friction_scale      =  0.50f;
    player->gravity_intensity   = -10.0f;

    player->dash_timer.duration = 0.5f;

    return(player);
}

internal void
entity_render(render_state_t *render_state, asset_manager_t *asset_manager, entity_t *entity, float32 alpha)
{
    if(alpha < 0.0f) alpha = 0.0f;
    if(alpha > 1.0f) alpha = 1.0f;
    entity->sprite = entity_init_sprite_data(asset_manager, entity->sprite.name);

    vec2_t render_pos = vec2_lerp(entity->prev_position, entity->position, alpha);
    r_draw_texture(render_state, 
                   render_pos, 
                   entity->render_size, 
                   entity->render_color, 
                   entity->rotation, 
                   entity->sprite.texture, 
                   (render_quad_options_t)entity->render_options);
}

internal void
game_simulate(vec2_t input_axis, float32 delta_time)
{
    // NOTE(Sleepster): entity simulate loop
    for(u32 entity_index = 0;
        entity_index <= global_game_state.entity_manager.active_entity_count;
        ++entity_index)
    {
        entity_t *entity = global_game_state.entity_manager.entities + entity_index;
        if((entity->e_flags & EF_Actor) != 0)
        {
            if(entity->e_type == ET_Player)
            {
                entity->prev_position = entity->position;
                if(entity->velocity.x != 0.0f)
                {
                    entity->facing_dir = entity->velocity.x > 0.0f ? 1 : -1;
                }

                if(entity->is_on_ground)
                {
                    if(input_axis.x != 0.0f)
                    {
                        entity->velocity.x += (input_axis.x * (entity->x_accel * delta_time));

                        if(entity->velocity.x >  entity->movement_speed) entity->velocity.x =  entity->movement_speed;
                        if(entity->velocity.x < -entity->movement_speed) entity->velocity.x = -entity->movement_speed;
                    }

                    if(global_game_state.should_jump)
                    {
                        entity->is_on_ground = false;
                        entity->velocity.y   = entity->jump_speed * delta_time; 

                        global_game_state.should_jump = false;
                    }
                }
                else
                {
                    if(global_game_state.should_dash)
                    {
                        entity->velocity.x = entity->facing_dir * (entity->dash_speed * delta_time);
                        global_game_state.should_dash = false;
                    }
                }

                if((entity->e_flags & EF_Gravitic) != 0)
                {
                    entity->velocity.y += (entity->gravity_intensity * delta_time);
                }

                entity->position = vec2_add(entity->position, entity->velocity);
                if(input_axis.x == 0.0f)
                {
                    vec2_t scaled_vel  = vec2_scale(entity->velocity, entity->friction_scale);
                    entity->velocity.x = scaled_vel.x;
                }

                if(entity->position.y <= 0.0f)
                {
                    entity->is_on_ground = true;
                    entity->position.y   = 0.0f;
                }

            }
        }
    }
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
#ifdef INTERNAL_DEBUG
    DEBUG_TIMED_BLOCK();
#endif

#if DEVELOPER_BUILD
    if(global_context == null)
    {
        global_context = context;
        DEBUG_global_state = DEBUG_global_state_in;
    }
#endif

    // NOTE(Sleepster): Initialize Gamestate
    if(!global_game_state.is_initialized) 
    {
        initialize_gamestate(render_state, input_manager, asset_manager);
        global_game_state.is_initialized = true;
    }
    //r_DEBUG_test_render(render_state, audio_manager, asset_manager, delta_time);

    // NOTE(Sleepster): Input Processing
    global_game_state.input_axis = {};
    {
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
        if(s_input_manager_is_keyboard_key_pressed(global_game_state.controller, SDL_SCANCODE_SPACE))
        {
            global_game_state.should_jump = true;
        }
        if(s_input_manager_is_keyboard_key_pressed(global_game_state.controller, SDL_SCANCODE_LSHIFT))
        {
            global_game_state.should_dash = true;
        }
        global_game_state.input_axis = vec2_normalize(global_game_state.input_axis);
    }

    // NOTE(Sleepster): Simulate Loop 
    {
        dt_accumulator += frame_time;
        if(frame_time >= (UPDATE_RATE * 2.0f))
        {
            frame_time = UPDATE_RATE * 2.0f;
        }

        while(dt_accumulator >= UPDATE_RATE)
        {
            game_simulate(global_game_state.input_axis, UPDATE_RATE);
            dt_accumulator -= UPDATE_RATE;
        }
    }
    float32 alpha = (dt_accumulator / UPDATE_RATE);

    // NOTE(Sleepster): Entity Render Loop 
    {
        r_renderpass_begin(render_state, global_game_state.entity_render_group);
        for(u32 entity_index = 0;
            entity_index <= global_game_state.entity_manager.active_entity_count;
            ++entity_index)
        {
            entity_t *entity = global_game_state.entity_manager.entities + entity_index;
            entity_render(render_state, asset_manager, entity, alpha);
        }
        r_renderpass_end(render_state);
    }
}

