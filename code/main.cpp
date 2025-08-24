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
#include "s_audio_manager.h"
#include "s_input_manager.h"
#include "at_atlas_handler.h"
#include "r_renderer_data.h"
#include "r_asset_shader.h"
#include "r_asset_texture.h"
#include "r_asset_dynamic_render_font.h"
#include "a_asset_loaded_sound.h"

#include "r_asset_shader.cpp"
#include "r_asset_texture.cpp"
#include "r_asset_dynamic_render_font.cpp"
//#include "r_asset_loaded_sound.c"
#include "s_asset_manager.cpp"
#include "s_audio_manager.cpp"
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
        s_input_manager_handle_window_inputs(&event, input_manager);
        switch(event.type)
        {
            case SDL_EVENT_QUIT:
            {
                running = false;
            }break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
            }break;
        }
    }
}

internal void
create_sine_wave(s16 *buffer, s32 sample_count)
{
    u32 tone_hz   = 300;
    u32 amplitude = 30000;
    for(s32 sample_index = 0;
        sample_index < sample_count;
        ++sample_index)
    {
        float32 time_value = (float32)sample_index / (float32)48000;

        s16 *sample0 = buffer + (sample_index * 2 + 0);
        s16 *sample1 = buffer + (sample_index * 2 + 1);
        *sample0 = (s16)((amplitude * sinf((2.0f * PI32) * tone_hz * time_value)) / 200.0);
        *sample1 = (s16)((amplitude * sinf((2.0f * PI32) * tone_hz * time_value)) / 200.0);
    }
}

int
main(int argc, char **argv)
{
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD|SDL_INIT_JOYSTICK) == 0)
    {
        log_fatal("Failed to start SDL critical subsystems...exit code: '%s'\n", SDL_GetError());
        return(-1);
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
        
        // NOTE(Sleepster): Audio Setup
        SDL_AudioDeviceID device             = 0;
        SDL_AudioSpec     device_spec        = {};
        SDL_AudioSpec     audio_manager_spec = {};
        SDL_AudioStream  *stream             = null;
        {
            device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, null);
            if(device != 0)
            {
                audio_manager_spec.freq     = 48000;
                audio_manager_spec.format   = SDL_AUDIO_S16LE;
                audio_manager_spec.channels = 2;
                
                s32 device_buffer_size_in_sample_frames;
                bool32 result = SDL_GetAudioDeviceFormat(device, &device_spec, &device_buffer_size_in_sample_frames);
                if(result)
                {
                    log_info("Device: '%s' opened. Device data format: '%d'. Device channel count: '%d', Device sample rate: '%d'...\n",
                             SDL_GetAudioDeviceName(device), device_spec.format, device_spec.channels, device_spec.freq);


                    stream = SDL_CreateAudioStream(&audio_manager_spec, &device_spec);
                    if(stream != null)
                    {
                        log_info("Successfully created a new SDL_AudioStream!\n");
                        bool32 did_bind = SDL_BindAudioStream(device, stream);
                        if(did_bind)
                        {
                            log_info("Bound the audio stream to the device: '%s'...\n",
                                     SDL_GetAudioDeviceName(device));

                            SDL_ResumeAudioDevice(device);
                        }
                        else
                        {
                            log_error("Failure to bind the audio stream to the primary device... error: '%s'...\n", SDL_GetError());
                        }
                    }
                }
                else
                {
                    log_error("Failure to open the system's default audio device... error: '%s'\n", SDL_GetError());
                }
            }
        }
        
        running = true;

        s32 bytes_per_sample         = sizeof(s16);
        s32 bytes_per_sample_frame   = 2.0f * bytes_per_sample; 
        s32 frames_to_write          = 12000;
        s32 bytes_to_write           = frames_to_write * bytes_per_sample_frame;
        s32 sample_count             = bytes_to_write / sizeof(s16);
        while(running)
        {
            s32 queued_audio = SDL_GetAudioStreamQueued(stream);
            if(queued_audio < bytes_to_write * 4)
            {
                s16 *buffer = (s16*)c_arena_push_size(&global_context.temporary_arena, bytes_to_write);
                if(buffer)
                {
                    create_sine_wave(buffer, sample_count);
                    bool32 result = SDL_PutAudioStreamData(stream, buffer, bytes_to_write);
                    if(!result)
                    {
                        log_error("Could not put SDL_AudioStream data... Error: %s'\n", SDL_GetError());
                    }
                }
            }
            
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
