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
#include "c_memory_arena.h"
#include "c_string.h"
#include "c_array.h"
#include "c_file_api.h"
#include "c_file_watcher.h"
#include "c_intrinsics.h"
#include "c_hash_table.h"
#include "c_multithreading_primitives.h"

#include "os_platform_file.h"

#include "c_zone_allocator.h"
#include "s_multithreading_work_queue.h"
#include "s_asset_manager.h"
#include "s_audio_manager.h"
#include "s_input_manager.h"
#include "at_atlas_handler.h"
#include "r_renderer_data.h"
#include "r_asset_shader.h"
#include "r_asset_texture.h"
#include "r_asset_dynamic_render_font.h"
#include "a_asset_loaded_sound.h"

#include "c_memory_arena.cpp"
#include "c_zone_allocator.cpp"
#include "c_string.cpp"
#include "c_array.cpp"
#include "c_file_api.cpp"
#include "c_file_watcher.cpp"
#include "c_hash_table.cpp"
#include "s_multithreading_work_queue.cpp"
#include "s_asset_manager.cpp"
#include "s_audio_manager.cpp"
#include "s_input_manager.cpp"
#include "at_atlas_handler.cpp"
#include "a_asset_loaded_sound.cpp"
#include "r_asset_shader.cpp"
#include "r_asset_texture.cpp"
#include "r_asset_dynamic_render_font.cpp"
#include "r_render_API.cpp"
#include "r_opengl.cpp"

#include "g_main.cpp"

global bool8 running;

#ifdef INTERNAL_DEBUG
FILE_WATCHER_CALLBACK(test_callback)
{
    asset_manager_t *asset_manager = (asset_manager_t *)user_data;
    log_info("Change data is for file: '%s'... Last modtime was: '%ul'...\n", change->full_path.data, change->last_change_timestamp);
    u32 ext = c_file_ext_string_to_enum(c_string_get_file_ext_from_path(change->full_path));

    hash_table_t *asset_table = null;
    bool8 is_asset = false;
    switch(ext)
    {
        case FILE_EXT_TTF:
        {
            asset_table = &asset_manager->font_catalog.font_hash;
            is_asset = true;
        }break;
        case FILE_EXT_WAV:
        {
            asset_table = &asset_manager->sound_catalog.sound_hash;
            is_asset = true;
        }break;
        case FILE_EXT_PNG:
        {
            asset_table = &asset_manager->texture_catalog.texture_hash;
            is_asset = true;
        }break;
        case FILE_EXT_GLSL:
        {
            //asset_table = &asset_manager->shader_catalog.shader_hash;
            is_asset = true;
        }break;
        case FILE_EXT_OS_DLL:
        {
            //asset_table = &asset_manager->shader_catalog.shader_hash;
        }break;
        default: {}break;
    }

    if(asset_table)
    {
        string_t filename     = c_string_get_filename_from_path_and_ext(change->full_path);
        string_t old_filename = c_string_get_filename_from_path_and_ext(change->old_filename);

        asset_slot_t *slot = (asset_slot_t *)c_hash_get_value(asset_table, filename);
        if(slot)
        {
            slot->slot_state = ASS_RELOADING;
            // NOTE(Sleepster): If the file has been moved, it's been renamed. Change the hash to reflect this.
            if((change->changes & FWC_EVENT_RENAMED) != 0)
            {
                u64 old_hash_value = c_hash_create_key_index(asset_table, old_filename.data, old_filename.count);
                slot->filename     = filename;
                c_hash_clear_index(asset_table, old_hash_value);

                c_hash_insert_kv_pair(asset_table, slot->filename, slot);
            }
        }
        else
        {
            log_info("Asset by name: '%s' cannot be found in this hash table...\n", change->full_path.data);
        }
    }
    else if(!asset_table && is_asset)
    {
        log_info("Could not find a valid asset table for asset: '%s'...\n", change->full_path.data);
    }
}
#endif

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

        multithreading_work_queue_manager_t work_manager = {};
        s_work_queue_manager_init(&work_manager);
        
        render_state_t render_state = {};
        r_init_renderer_data(window, &render_state);

        asset_manager_t asset_manager = {};
        s_asset_manager_init(&asset_manager, STR("../build/asset_file.wad"));
        asset_manager.queue_manager = &work_manager;

        input_manager_t input_manager = {};
        s_input_manager_initialize_keyboard_controller(&input_manager, 0);

        audio_manager_t audio_manager = {};
        s_audio_manager_init(&audio_manager);

        u64 performance_counter_frequency = SDL_GetPerformanceFrequency();
        u64 last_tsc                      = SDL_GetPerformanceCounter();
        u64 current_tsc                   = 0;
        u64 delta_tsc                     = 0;

        float64 delta_time = 0.0f;

        file_watcher_t watcher = c_file_watcher_create(FWC_EVENT_ALL, true, test_callback, &asset_manager, false);
        c_file_watcher_add_path(&watcher, STR("../run_tree/res/"));
        c_file_watcher_issue_check_over_all_paths(&watcher);

        running = true;
        while(running)
        {
            c_file_watcher_process_changes(&watcher);

            s_input_manager_reset_controller_states(&input_manager);
            c_process_window_events(&input_manager);

            g_update_and_render(&render_state, &audio_manager, &asset_manager);
            s_audio_manager_fill_sound_buffer(&asset_manager, &audio_manager, delta_time);

            r_render_single_frame(&asset_manager, &render_state);
            SDL_GL_SwapWindow(window);

            c_arena_reset(&render_state.draw_frame_arena);
            ZeroStruct(render_state.draw_frame);
            gc_reset_temporary_data();

            current_tsc = SDL_GetPerformanceCounter();
            delta_tsc   = current_tsc - last_tsc;
            last_tsc    = current_tsc;

            delta_time  = (float32)((float64)delta_tsc / (float64)performance_counter_frequency);
            //log_info("Delta time in seconds: '%.03f'...\n", delta_time);
        }
    }

    return(0);
}
