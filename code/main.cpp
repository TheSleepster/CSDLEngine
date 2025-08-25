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

        audio_manager_t audio_manager = {};
        s_audio_manager_init(&audio_manager);

        u64 performance_counter_frequency = SDL_GetPerformanceFrequency();
        u64 last_tsc                      = SDL_GetPerformanceCounter();
        u64 current_tsc                   = 0;
        u64 delta_tsc                     = 0;

        float64 delta_time = 0.0f;

        running = true;
        while(running)
        {
            s32 sample_rate      = audio_manager.audio_manager_spec.freq;
            s32 bytes_per_sample = audio_manager.audio_manager_spec.channels * sizeof(s16);
            
            float32 device_latency = audio_manager.current_playback_device.device_buffer_size_ms;

            s32 samples_to_write = (s32)(ceilf(((float32)device_latency * (float32)sample_rate) / 1000.0f));
            s32 bytes_to_write   = samples_to_write * bytes_per_sample;
            
            s32 queued_audio = SDL_GetAudioStreamQueued(audio_manager.stream);
            if(queued_audio < bytes_to_write)
            {
                audio_manager.buffer.sample_buffer = (s16*)c_arena_push_size(&global_context.temporary_arena,
                                                                              bytes_to_write);
                if(audio_manager.buffer.sample_buffer)
                {
                    create_sine_wave(audio_manager.buffer.sample_buffer, samples_to_write);
                    bool32 result = SDL_PutAudioStreamData(audio_manager.stream,
                                                           audio_manager.buffer.sample_buffer,
                                                           bytes_to_write);
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

            current_tsc = SDL_GetPerformanceCounter();
            delta_tsc   = current_tsc - last_tsc;
            last_tsc    = current_tsc;

            delta_time  = (float32)((float64)delta_tsc / (float64)performance_counter_frequency);
            log_info("Delta time in seconds: '%.03f'...\n", delta_time);
        }
    }

    return(0);
}
