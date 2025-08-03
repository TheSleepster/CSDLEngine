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

#include "c_memory.c"
#include "c_string.c"
#include "c_array.c"
#include "c_file_api.c"
#include "c_hash_table.c"

#include "s_asset_manager.h"
#include "r_renderer_data.h"
#include "r_asset_shader.h"
#include "r_asset_texture.h"
//#include "r_asset_dynamic_font.h"

#include "r_asset_shader.c"
#include "r_asset_texture.c"
//#include "r_asset_dynamic_font.c"
#include "s_asset_manager.h"
#include "r_render_API.c"
#include "r_opengl.c"

#include "g_main.c"

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

    SDL_Window *window = SDL_CreateWindow("SDL Window", 1920, 1080, SDL_WINDOW_OPENGL);
    if(window)
    {
        gc_setup();
        
        render_state_t render_state = {};
        r_init_renderer_data(window, &render_state);

        bool8 game_initialized = false;

        running = true;
        while(running)
        {
            c_process_window_events();

            g_update_and_render(game_initialized, &render_state);

            r_render_single_frame(&render_state);
            SDL_GL_SwapWindow(window);

            c_arena_reset(&render_state.draw_frame_arena);
            ZeroStruct(render_state.draw_frame);

            gc_reset_temporary_data();
        }
    }

    return(0);
}
