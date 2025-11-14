/* ========================================================================
   $File: g_main.c $
   $Date: Wed, 30 Jul 25: 05:28PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "l_runtime_data.cpp"

#define COLOR_WHITE  ((vec4_t){1.0, 1.0, 1.0, 1.0})
#define COLOR_RED    ((vec4_t){1.0, 0.0, 0.0, 1.0})
#define COLOR_GREEN  ((vec4_t){0.0, 1.0, 0.0, 1.0})
#define COLOR_BLUE   ((vec4_t){0.0, 0.0, 1.0, 1.0})
#define COLOR_BLACK  ((vec4_t){0.0, 0.0, 0.0, 1.0})

constexpr float32 GRAVITY_A      = 1.5f;
constexpr float32 UPDATE_RATE    = 1.0f / 60.0f; 
constexpr u32     MAX_ENTITIES   = 10000;

constexpr u32 TILE_SIZE = 8;

global float32 dt_accumulator = 0.0f;

struct tick_clock_t
{
    bool32 begun;

    u32    start_tick;
    u32    end_tick;

    u32    elapsed;
    u32    duration;
};

struct sprite_t
{
    asset_handle_t texture;
    vec2_t         size;
    u32            frame_count;

    string_t       name;
};

enum e_collision_masking
{
    ECM_Player      = 1ul << 0,
    ECM_Map         = 1ul << 1,
    ECM_AllEntities = 1ul << 2,
};

struct collision_box
{
    bool32       is_active;
    u32          collision_mask;
    rectangle2_t rect;
};

enum entity_type
{
    ET_Player,
    ET_Tile,
    ET_Count
};

enum entity_flags
{
    EF_Valid    = 1ul << 0,
    EF_Alive    = 1ul << 1,
    EF_Gravitic = 1ul << 2,
    EF_Actor    = 1ul << 3,
    EF_Static   = 1ul << 4,
    EF_IsGround = 1ul << 5,
};

struct entity_t
{
    u64           e_flags;
    u32           e_ID;
    u32           e_type;
    u32           render_options;

    collision_box hit_box;
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
 
    tick_clock_t  dash_clock;
    u32           dash_counter;
    u32           max_dashes;

    u32           jump_counter;
    u32           max_jumps;

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

#include "g_test.cpp"
#include "g_editor.cpp"

struct game_state_t 
{
    bool8               is_initialized;
    input_controller_t *controller;
    render_group_t     *entity_render_group;

    entity_manager_t    entity_manager;
    game_map_editor_t   map_editor;

    u32                 physics_iterations;

    entity_t           *player;
    vec2_t              input_axis;
    bool8               should_jump;
    bool8               should_dash;
    u64                 simulation_tick;

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

    u32 entity_id = 0;
    for(u32 entity_index = 0;
        entity_index < MAX_ENTITIES;
        ++entity_index)
    {
        entity_t *entity = entity_manager->entities + entity_index;
        if(!(entity->e_flags & EF_Valid))
        {
            new_entity = entity;
            entity_id  = entity_index;
            break;
        }
    }
    Assert(new_entity);

    ZeroStruct(*new_entity);
    new_entity->e_ID    = entity_id;
    new_entity->e_flags = EF_Valid;

    ++entity_manager->active_entity_count;
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
entity_create_player(entity_manager_t *entity_manager, asset_manager_t *asset_manager, vec2_t position)
{
    entity_t *player = entity_create(entity_manager);

    player->e_type                = ET_Player;
    player->e_flags              |= (EF_Alive|EF_Gravitic|EF_Actor);
    player->sprite                = entity_init_sprite_data(asset_manager, STR("player"));
    player->render_size           = vec2(10, 10);
    player->render_color          = COLOR_WHITE;
    player->position              = position;

    player->hit_box.rect           = rect2_create(player->position, player->render_size);
    player->hit_box.collision_mask = ECM_AllEntities;
    player->hit_box.is_active      = true;

    player->x_accel               =  20.0f;
    player->movement_speed        =  2.0f;
    player->terminal_velocity     =  5.0f; 
    player->jump_speed            =  300.0f;
    player->dash_speed            =  300.0f;
    player->friction_scale        =  0.50f;
    player->gravity_intensity     = -10.0f;

    player->dash_clock.duration   = 15;
    player->dash_counter          = 1;
    player->max_dashes            = 1;

    player->jump_counter          = 1;
    player->max_jumps             = 1;

    return(player);
}

internal entity_t*
entity_create_test_tile(entity_manager_t *entity_manager, asset_manager_t *asset_manager, vec2_t position)
{
    entity_t *tile = entity_create(entity_manager);

    tile->e_type                 = ET_Tile;
    tile->e_flags               |= EF_Static;
    tile->sprite                 = entity_init_sprite_data(asset_manager, STR("textureless_sprite"));
    tile->render_size            = vec2(TILE_SIZE, TILE_SIZE);
    tile->render_color           = COLOR_RED;
    tile->position               = position;

    tile->hit_box.rect           = rect2_create(tile->position, tile->render_size);
    tile->hit_box.collision_mask = ECM_Player;
    tile->hit_box.is_active      = true;

    return(tile);
}

internal void
entity_render(render_state_t *render_state, asset_manager_t *asset_manager, entity_t *entity, float32 alpha)
{
    if(entity->e_flags & EF_Valid)
    {
        vec2_t render_pos = entity->position;
        if((entity->e_flags & EF_Actor) != 0)
        {
            alpha = Clamp(alpha, 0.0f, 1.0f);
            render_pos = vec2_lerp(entity->prev_position, entity->position, alpha);
        }

        entity->sprite = entity_init_sprite_data(asset_manager, entity->sprite.name);
        r_draw_texture(render_state, 
                       render_pos, 
                       entity->render_size, 
                       entity->render_color, 
                       entity->rotation, 
                       entity->sprite.texture, 
                       (render_quad_options_t)entity->render_options);
    }
}

internal void
entity_update_player(entity_t *entity, vec2_t input_axis, float32 delta_time)
{
    if(entity->is_on_ground)
    {
        global_game_state.should_dash = false;
        entity->dash_clock.begun      = false;
        if(input_axis.x != 0.0f)
        {
            entity->velocity.x += (input_axis.x * (entity->x_accel * delta_time));

            if(entity->velocity.x >  entity->movement_speed) entity->velocity.x =  entity->movement_speed;
            if(entity->velocity.x < -entity->movement_speed) entity->velocity.x = -entity->movement_speed;
        }

        if(global_game_state.should_jump && entity->jump_counter > 0)
        {
            entity->is_on_ground = false;
            entity->velocity.y   = entity->jump_speed * delta_time; 

            --entity->jump_counter;
            global_game_state.should_jump = false;
        }

        if(input_axis.x == 0.0f)
        {
            vec2_t scaled_vel  = vec2_scale(entity->velocity, entity->friction_scale);
            entity->velocity.x = scaled_vel.x;
        }
    }
    else
    {
        if(global_game_state.should_dash && entity->dash_counter > 0)
        {
            if(!entity->dash_clock.begun)
            {
                entity->dash_clock.start_tick = global_game_state.simulation_tick;
                entity->dash_clock.elapsed    = 0;
                entity->dash_clock.begun      = true;
            }

            if(entity->dash_clock.elapsed < entity->dash_clock.duration)
            {
                entity->velocity.x = entity->facing_dir * (entity->dash_speed * delta_time);
                entity->velocity.y = input_axis.y * (entity->dash_speed * delta_time);

                entity->dash_clock.elapsed += 1;
            }
            else
            {
                global_game_state.should_dash = false;
                entity->dash_clock.begun      = false;

                --entity->dash_counter;
            }
        }

        if(input_axis.x != 0.0f)
        {
            entity->velocity.x += (input_axis.x * ((entity->x_accel * 0.25) * delta_time));
        }


        if(input_axis.x == 0.0f)
        {
            vec2_t scaled_vel  = vec2_scale(entity->velocity, entity->friction_scale * 2.0f);
            entity->velocity.x = scaled_vel.x;
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
    global_game_state.player = entity_create_player(&global_game_state.entity_manager, asset_manager, vec2_zero());

    entity_create_test_tile(&global_game_state.entity_manager, asset_manager, vec2(-16, 0));
}


internal void
game_simulate(vec2_t input_axis, float32 delta_time)
{
    DEBUG_TIMED_BLOCK(); 
    ++global_game_state.simulation_tick;

    // NOTE(Sleepster): Entity Interaction Loop 
    for(u32 entity_index = 0;
        entity_index < global_game_state.entity_manager.active_entity_count;
        ++entity_index)
    {
        entity_t *entity = global_game_state.entity_manager.entities + entity_index;
        if((entity->e_flags & EF_Actor) != 0)
        {
            entity->prev_position = entity->position;
            if(entity->velocity.x != 0.0f)
            {
                entity->facing_dir = entity->velocity.x > 0.0f ? 1 : -1;
            }

            // TODO(Sleepster): switch(entity->e_type) 
            if(entity->e_type == ET_Player)
            {
                entity_update_player(entity, input_axis, delta_time);
            }

            if(((entity->e_flags & EF_Gravitic) != 0) && !entity->is_on_ground)
            {
                entity->velocity.y += (entity->gravity_intensity * delta_time);
            }
        }
    }

    // NOTE(Sleepster): Broad Phase Collision Detection 
    for(u32 entity_index = 0;
        entity_index < global_game_state.entity_manager.active_entity_count;
        ++entity_index)
    {
         // TODO(Sleepster): this 
    }

    // NOTE(Sleepster): Discrete Entity Collision Detection loop
    for(u32 entity_index = 0;
        entity_index < global_game_state.entity_manager.active_entity_count;
        ++entity_index)
    {
        entity_t *entity = global_game_state.entity_manager.entities + entity_index;
        if(!(entity->e_flags & EF_Actor) || !entity->hit_box.is_active) 
        {
            continue;
        }

        vec2_t desired_movement = entity->velocity;

        for(u32 test_index = 0;
            test_index < global_game_state.entity_manager.active_entity_count;
            ++test_index)
        {
            entity_t *test_entity = global_game_state.entity_manager.entities + test_index;
            if(test_entity->e_ID == entity->e_ID || !test_entity->hit_box.is_active) 
            {
                continue;
            }

            // NOTE(Sleepster): Sweep Response
            {
                raytest_t sweep = rect2_sweep_test(entity->hit_box.rect, desired_movement, test_entity->hit_box.rect);
                if(sweep.hit)
                {
                    desired_movement = vec2_scale(desired_movement, sweep.time);

                    if(sweep.normal.x != 0.0f) entity->velocity.x = 0.0f;
                    if(sweep.normal.y != 0.0f) entity->velocity.y = 0.0f;
                }
            }

            // NOTE(Sleepster): Stationary Response
            {
                rectangle2_t predicted_hitbox = entity->hit_box.rect;
                rect2_shift_by(&predicted_hitbox, desired_movement);

                rectangle2_t minkowski = rect2_minkowski_difference(predicted_hitbox, test_entity->hit_box.rect);
                if(minkowski.min.x <= 0 && minkowski.max.x >= 0 && 
                   minkowski.min.y <= 0 && minkowski.max.y >= 0)
                {
                    vec2_t overlap_vector = rect2_get_vector_depth(minkowski);
                    desired_movement      = vec2_add(desired_movement, overlap_vector);

                    if(overlap_vector.x != 0) entity->velocity.x = 0.0f;
                    if(overlap_vector.y != 0) entity->velocity.y = 0.0f;
                }
            }
        }

        entity->position     = vec2_add(entity->position, desired_movement);
        entity->hit_box.rect = rect2_create(entity->position, entity->render_size);
    }

    // NOTE(Sleepster): Update actor positions after Discrete/Broad phase collision checks. 
    for(u32 entity_index = 0;
        entity_index < global_game_state.entity_manager.active_entity_count;
        ++entity_index)
    {
        entity_t *entity = global_game_state.entity_manager.entities + entity_index;
        if(entity->e_flags & EF_Actor)
        {
            if(entity->position.y <= 0.0f)
            {
                entity->is_on_ground = true;
                entity->position.y   = 0.0f;
                entity->velocity.y   = 0.0f;

                entity->jump_counter = entity->max_jumps;
                entity->dash_counter = entity->max_dashes;
            }
        }
    }
}

GAME_API external
GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    DEBUG_TIMED_BLOCK();

#if PROFILER_ENABLED 
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
    //r_DEBUG_test_render(render_state, audio_manager, asset_manager, UPDATE_RATE);

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

        if(s_input_manager_is_keyboard_key_pressed(global_game_state.controller, SDL_SCANCODE_E) &&
           s_input_manager_is_control_key_down(global_game_state.controller))
        {
            global_game_state.in_editor = !global_game_state.in_editor;
        }

        global_game_state.input_axis = vec2_normalize(global_game_state.input_axis);
    }

    // NOTE(Sleepster): Simulate Loop 
    {
        if(!global_game_state.in_editor)
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
        else
        {
            game_editor_update(render_state, &global_game_state.map_editor);
        }
    }
    float32 alpha = (dt_accumulator / UPDATE_RATE);

    // NOTE(Sleepster): Entity Render Loop 
    {
        r_renderpass_begin(render_state, global_game_state.entity_render_group);
        for(u32 entity_index = 0;
            entity_index < global_game_state.entity_manager.active_entity_count;
            ++entity_index)
        {
            entity_t *entity = global_game_state.entity_manager.entities + entity_index;
            entity_render(render_state, asset_manager, entity, alpha);

            // NOTE(Sleepster): Draws colliders 
            //r_draw_rect(render_state, entity->hit_box.rect.min, entity->render_size, COLOR_BLUE, 0, RQO_NONE);
        }
        r_renderpass_end(render_state);
    }

}

