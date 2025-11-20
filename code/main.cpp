/* ========================================================================
   $File: main.cpp $
   $Date: July 22 2025 02:44 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "l_runtime_data.cpp"

#if DLL_RELOADING
    #define game_update_and_render(...) game_functions.update_and_render(__VA_ARGS__) 
    GAME_UPDATE_AND_RENDER(g_update_and_render_stub)
    {
    }
#else
    #include "g_main.cpp"
#endif

struct game_dll_data_t
{
    void                     *game_lib;
    game_update_and_render_t *update_and_render; 
};

internal void
DEBUG_game_load_library(game_dll_data_t *game_data)
{
#if OS_WINDOWS
    string_t game_dll      = STR("game_debug.dll");
    string_t game_copy_dll = STR("game_debug_COPY.dll");
#else
    string_t game_dll      = STR("./game_debug.so");
    string_t game_copy_dll = STR("./game_debug_COPY.so");
#endif
    c_file_copy(game_dll, game_copy_dll);

    game_data->game_lib = os_load_library(game_copy_dll);
}

internal void
DEBUG_get_game_functions(game_dll_data_t *game_data, GPU_functions_t *gpu_functions)
{
#if DLL_RELOADING
    DEBUG_game_load_library(game_data);
    if(game_data->game_lib)
    {
        game_data->update_and_render = (game_update_and_render_t *)os_get_proc_address(game_data->game_lib, STR("game_update_and_render"));
        if(!game_data->update_and_render)
        {
            log_fatal("Failure to acquire the game function 'g_update_and_render'...\n");
        }
    }
    else
    {
        log_fatal("Failure to load the game functions...\n");
        game_data->update_and_render = g_update_and_render_stub;
    }
#else
    game_data->update_and_render = game_update_and_render;
#endif

    if(gpu_functions)
    {
        gpu_functions->r_texture_make_gpu           = r_texture_make_gpu_; 
        gpu_functions->r_texture_delete             = r_texture_delete_; 
        gpu_functions->r_texture_update_from_bitmap = r_texture_update_from_bitmap_;
    }
}

internal void
DEBUG_reload_game_functions(game_dll_data_t *game_data)
{
    SDL_Delay(250);
    if(game_data->game_lib)
    {
        game_data->update_and_render = null;
        os_free_library(game_data->game_lib);

        DEBUG_get_game_functions(game_data, null);
        DEBUG_global_state->should_reload_dll = false;
    }
    else
    {
        DEBUG_game_load_library(game_data);
    }
}

FILE_WATCHER_CALLBACK(test_callback)
{
    asset_manager_t *asset_manager = (asset_manager_t *)user_data;
    log_info("Change data is for file: '%s'... Last modtime was: '%ul'...\n", change->full_path.data, change->last_change_timestamp);
    string_t file_ext = c_string_get_file_ext_from_path(change->full_path);
    u32 ext_type = c_file_ext_string_to_enum(file_ext);

    string_t filename     = c_string_get_filename_from_path_and_ext(change->full_path);
    string_t old_filename = c_string_get_filename_from_path_and_ext(change->old_filename);

    hash_table_t *asset_table = null;
    bool8 is_asset            = false;
    switch(ext_type)
    {
        case FILE_EXT_TTF:
        {
            asset_table = &asset_manager->font_catalog.font_hash;
            is_asset    = true;
        }break;
        case FILE_EXT_WAV:
        {
            asset_table = &asset_manager->sound_catalog.sound_hash;
            is_asset    = true;
        }break;
        case FILE_EXT_PNG:
        {
            asset_table = &asset_manager->texture_catalog.texture_hash;
            is_asset    = true;
        }break;
        case FILE_EXT_GLSL:
        {
            //asset_table = &asset_manager->shader_catalog.shader_hash;
            is_asset = true;
        }break;
        case FILE_EXT_OS_DLL:
        {
#if DLL_RELOADING
#if OS_WINDOWS
            string_t game_dll_name = STR("game_debug.dll");
            filename.count += 4;
#else
            string_t game_dll_name = STR("game_debug.so");
            filename.count += 3;
#endif
            if(c_string_compare(filename, game_dll_name))
            {
                DEBUG_global_state->should_reload_dll = true;
            }
#endif
        }break;
        default: {}break;
    }

    if(asset_table)
    {
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

internal void
c_process_window_events(SDL_Window *window, input_manager_t *input_manager)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        s_input_manager_handle_window_inputs(&event, input_manager);
        switch(event.type)
        {
            case SDL_EVENT_QUIT:
            {
                global_context->running = false;
            }break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                s32 window_x = 0;
                s32 window_y = 0;
                SDL_GetWindowSizeInPixels(window, &window_x, &window_y);

                global_context->window_size.x = (float32)window_x;
                global_context->window_size.y = (float32)window_y;
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
        global_context->window_size = vec2(1920.0f, 1080.0f);

        asset_manager_t asset_manager = {};
#if DLL_RELOADING
        GPU_functions_t GPU_functions  = {};
        game_dll_data_t game_functions = {};
        DEBUG_get_game_functions(&game_functions, &GPU_functions);
        asset_manager.gpu_data = &GPU_functions;
#endif
        
        render_state_t render_state = {};
        r_init_renderer_data(window, &render_state);

        input_manager_t input_manager = {};
        s_input_manager_initialize_keyboard_controller(&input_manager, 0);

        multithreading_work_queue_manager_t work_manager = {};
        s_work_queue_manager_init(&work_manager);

#if defined(PROFILER_ENABLED) || defined(DLL_RELOADING)
        DEBUG_create_debug_state(&render_state, &input_manager, &asset_manager);
#endif
        s_asset_manager_init(&asset_manager, STR("asset_file.wad"));
        asset_manager.queue_manager = &work_manager;

        audio_manager_t audio_manager = {};
        s_audio_manager_init(&audio_manager);

#if PROFILER_ENABLED 
        ui_init_state(&render_state, &input_manager, &asset_manager, &DEBUG_global_state->UI_data);
#endif

        u64 performance_counter_frequency = SDL_GetPerformanceFrequency();
        u64 last_tsc                      = SDL_GetPerformanceCounter();
        u64 current_tsc                   = 0;
        u64 delta_tsc                     = 0;
        float64 delta_time                = 0.0;
        float64 frame_time_in_ms          = 0.0;

        file_watcher_t watcher = c_file_watcher_create(FWC_EVENT_ALL, true, test_callback, &asset_manager, false);
        c_file_watcher_add_path(&watcher, STR("../res/"));
        c_file_watcher_issue_check_over_all_paths(&watcher);

        global_context->running = true;
        while(global_context->running)
        {
            //r_reset_draw_frame_pipeline_state(&render_state);
            s_input_manager_reset_controller_states(&input_manager);
            c_process_window_events(window, &input_manager);
            c_file_watcher_process_changes(&watcher);

            s_audio_manager_fill_sound_buffer(&asset_manager, &audio_manager, delta_time);
#if PROFILER_ENABLED
            game_update_and_render(global_context, &render_state, &audio_manager, &asset_manager, &input_manager, delta_time, DEBUG_global_state);
            DEBUG_render_group_to_output(&input_manager, &asset_manager, &render_state, frame_time_in_ms);
#else
            game_update_and_render(global_context, &render_state, &audio_manager, &asset_manager, &input_manager, delta_time);
#endif

            r_render_single_frame(&asset_manager, &render_state);
            SDL_GL_SwapWindow(window);

            c_arena_reset(&render_state.frame_arena);
            ZeroStruct(render_state.draw_frame);
            gc_reset_temporary_data();

            current_tsc = SDL_GetPerformanceCounter();
            delta_tsc   = current_tsc - last_tsc;
            last_tsc    = current_tsc;

            delta_time       = (float32)(((float64)delta_tsc) / (float64)performance_counter_frequency);
            frame_time_in_ms = delta_time * 1000; 
            //log_info("Delta time in seconds: '%.03f'...\n", delta_time);

#if PROFILER_ENABLED 
            DEBUG_set_event_marker(DEBUG_EVENT_FRAME_END);
            DEBUG_handle_events(&input_manager);
    #if DLL_RELOADING
            if(DEBUG_global_state->should_reload_dll)
            {
                DEBUG_reload_game_functions(&game_functions);

                s_work_queue_finish_all_work(&work_manager.high_priority_queue);
                s_work_queue_finish_all_work(&work_manager.low_priority_queue);
                //DEBUG_reset_state(&DEBUG_global_state);
            }
    #endif
#endif
        }
    }
    else
    {
        log_error("Cannot open an SDL_Window... Error: %s\n", SDL_GetError());
    }

    return(0);
}
