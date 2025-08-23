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
#include "at_atlas_handler.cpp"
#include "r_render_API.cpp"
#include "r_opengl.cpp"

#include "g_main.cpp"

global bool8 running;

internal void
c_process_window_events()
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
        }
    }
}

int
main(int argc, char **argv)
{
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) == 0)
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

        running = true;
        while(running)
        {
            c_process_window_events();

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
