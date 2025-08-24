/* ========================================================================
   $File: main.c $
   $Date: July 22 2025 02:44 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include <SDL3/SDL.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb/stb_image.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "c_base.h"
#include "c_types.h"
#include "c_math.h"
#include "c_debug.h"
#include "c_memory.h"
#include "c_string.h"
#include "c_array.h"
#include "c_file_api.h"
#include "c_intrinsics.h"
#include "c_hash_table.h"

#include "os_platform_file.h"

#include "c_memory.cpp"
#include "c_string.cpp"
#include "c_array.cpp"
#include "c_file_api.cpp"
#include "c_hash_table.cpp"

#include "s_asset_manager.h"
#include "at_atlas_handler.h"
#include "r_renderer_data.h"
#include "r_asset_shader.h"
#include "r_asset_texture.h"
#include "r_asset_dynamic_render_font.h"
//#include "r_asset_loaded_sound.h"

#include "r_asset_shader.cpp"
#include "r_asset_texture.cpp"
#include "r_asset_dynamic_render_font.cpp"
//#include "r_asset_loaded_sound.c"
#include "s_asset_manager.cpp"
#include "s_input_manager.cpp"
#include "at_atlas_handler.cpp"
#include "r_render_API.cpp"
#include "r_opengl.cpp"

#include "g_main.cpp"

global bool8 running;

internal void
c_process_window_events(input_manager_t *input_manager)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_EVENT_QUIT:
            {
                running = false;
            }break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
            }break;
            case SDL_EVENT_GAMEPAD_ADDED:
            {
                input_controller_t new_controller = {};
                new_controller.is_analog = true;
                new_controller.is_valid  = true;
                new_controller.type      = IM_CONTROLLER_GAMEPAD;

                if(SDL_IsGamepad(event.gdevice.which))
                {
                    new_controller.gamepad.gamepad_id = event.gdevice.which;
                    
                    new_controller.gamepad.gamepad_data = SDL_OpenGamepad(new_controller.gamepad.gamepad_id);
                    new_controller.gamepad.stick_data   = SDL_GetGamepadJoystick(new_controller.gamepad.gamepad_data);
                    new_controller.gamepad.has_rumble   = SDL_RumbleGamepad(new_controller.gamepad.gamepad_data, 0xffff, 0xffff, 100000);

                    if(input_manager->connected_controller_count < MAX_INPUT_CONTROLLERS)
                    {
                        log_info("Controller '%s' connected...\n", SDL_GetGamepadName(new_controller.gamepad.gamepad_data));

                        input_manager->controllers[input_manager->connected_controller_count] = new_controller;
                        input_manager->connected_controller_count += 1;
                    }
                    else
                    {
                        log_info("Unable to connect gamepad device... Maximum controller count of: '%d' has been reached...\n", MAX_INPUT_CONTROLLERS);
                    }
                }
            }break;
            case SDL_EVENT_GAMEPAD_REMOVED:
            {
                u32 controller_id = event.gdevice.which;
                for(u32 controller_index = 0;
                    controller_index < MAX_INPUT_CONTROLLERS;
                    ++controller_index)
                {
                    input_controller_t *controller = input_manager->controllers + controller_index;
                    if(controller->type == IM_CONTROLLER_GAMEPAD &&
                       controller->gamepad.gamepad_id == controller_id)
                    {
                        if(controller_index == input_manager->primary_controller_index)
                        {
                            input_manager->primary_controller_index = 0;
                        }
                        if(controller_index == input_manager->active_controller_index)
                        {
                            input_manager->active_controller_index = 0;
                        }

                        SDL_CloseGamepad(controller->gamepad.gamepad_data);
                        SDL_CloseJoystick(controller->gamepad.stick_data);

                        ZeroStruct(*controller);
                        input_manager->connected_controller_count -= 1;

                        break;
                    }
                }
            }break;
            case SDL_EVENT_KEY_UP:
            case SDL_EVENT_KEY_DOWN:
            {
                input_controller_t *controller = input_manager->controllers;
                Assert(controller->type == IM_CONTROLLER_KEYBOARD);

                u32 key_index = event.key.scancode;

                action_button_t *action_key = controller->keyboard.input + key_index;
                action_key->is_pressed      = (event.key.down && !event.key.repeat);
                action_key->is_down         = (event.key.down && event.key.repeat);
                action_key->is_released     = (event.key.down == false);

                if(action_key->is_pressed || action_key->is_released)
                {
                    action_key->half_transition_counter += 1;
                }
            }break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                input_controller_t *controller = input_manager->controllers;
                Assert(controller->type == IM_CONTROLLER_KEYBOARD);

                float32 old_mouse_pos_x = controller->keyboard.current_mouse_pos.x; 
                float32 old_mouse_pos_y = controller->keyboard.current_mouse_pos.y;

                controller->keyboard.current_mouse_pos.x = event.motion.x;
                controller->keyboard.current_mouse_pos.y = event.motion.y;
                
                controller->keyboard.mouse_delta = vec2_subtract(controller->keyboard.current_mouse_pos, vec2_create_float(old_mouse_pos_x, old_mouse_pos_y));
            }break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                input_controller_t *controller = input_manager->controllers;
                Assert(controller->type == IM_CONTROLLER_KEYBOARD);

                u32 key_index = event.button.button + SDL_SCANCODE_MAX;

                action_button_t *button = controller->keyboard.input + key_index;
                button->is_pressed  = (event.button.down == true);
                button->is_released = (event.button.down == false);
                button->is_down     = ((event.button.down == true) && (button->half_transition_counter <= 1));

                button->half_transition_counter += event.button.clicks;
            }break;

            case SDL_EVENT_GAMEPAD_BUTTON_UP:
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            {
                input_controller_t *controller = input_manager->controllers + input_manager->active_controller_index;
                action_button_t *button = controller->gamepad.digital_buttons + event.gbutton.button;
                
                SDL_GamepadButtonEvent button_data = event.gbutton; 
                button->is_pressed  = ((button_data.down == true) && (button->half_transition_counter <= 1));
                button->is_down     = (button_data.down  == true);
                button->is_released = (button_data.down  == false);
            }break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            {
                input_controller_t *controller    = input_manager->controllers + input_manager->active_controller_index;
                analog_button_t    *analog_button = controller->gamepad.analog_buttons + event.gaxis.axis;

                analog_button->value = event.gaxis.value;
            }break;
        }
    }
}

int
main(int argc, char **argv)
{
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD|SDL_INIT_JOYSTICK) == 0)
    {
        log_fatal("Failed to start SDL critical subsystems...exiting\n");
        return(0);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,          1);

    SDL_Window *window = SDL_CreateWindow("SDL Window", 1920, 1080, SDL_WINDOW_OPENGL);
    if(window)
    {
        gc_setup();
        
        render_state_t render_state = {};
        r_init_renderer_data(window, &render_state);

        asset_manager_t asset_manager = {};
        s_asset_manager_init(&asset_manager, STR("../build/asset_file.wad"));

        input_manager_t input_manager = {};
        s_input_manager_initialize_keyboard_controller(&input_manager, 0);

        running = true;
        while(running)
        {
            s_input_manager_reset_controller_states(&input_manager);
            c_process_window_events(&input_manager);

            g_update_and_render(&render_state, &asset_manager);

            r_render_single_frame(&asset_manager, &render_state);
            SDL_GL_SwapWindow(window);

            c_arena_reset(&render_state.draw_frame_arena);
            ZeroStruct(render_state.draw_frame);

            gc_reset_temporary_data();
        }
    }

    return(0);
}
