/* ========================================================================
   $File: main.c $
   $Date: July 22 2025 02:44 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#include <stdio.h>

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include "c_base.h"
#include "c_types.h"
#include "c_math.h"
#include "c_debug.h"
#include "c_memory.h"
#include "c_string.h"
#include "c_array.h"
#include "c_file_api.h"
#include "c_intrinsics.h"

#include "os_platform_file.h"

#include "c_memory.c"
#include "c_string.c"
#include "c_array.c"
#include "c_file_api.c"

#include "r_renderer_data.h"

#include "r_render_API.c"
#include "r_opengl.c"

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
        render_state_t render_state = {};
        r_init_renderer_data(window, &render_state);

        running = true;
        while(running)
        {
            c_process_window_events();
            r_render_single_frame(window, &render_state);

            c_arena_reset(&render_state.draw_frame_arena);
            ZeroStruct(render_state.draw_frame);
        }
    }

    return(0);
}
