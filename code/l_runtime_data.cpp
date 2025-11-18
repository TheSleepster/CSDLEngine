#if !defined(L_RUNTIME_DATA_CPP)
/* ========================================================================
   $File: l_runtime_data.cpp $
   $Date: Sat, 06 Sep 25: 07:19PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define L_RUNTIME_DATA_CPP

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
#include "c_log_assert.h"
#include "c_memory_arena.h"
#include "c_string.h"
#include "c_array.h"
#include "c_file_api.h"
#include "c_file_watcher.h"
#include "c_intrinsics.h"
#include "c_hash_table.h"
#include "c_sorting.h"
#include "c_multithreading_primitives.h"

#include "os_platform_file.h"

#include "c_zone_allocator.h"
#include "s_multithreading_work_queue.h"
#include "s_asset_manager.h"
#include "s_audio_manager.h"
#include "s_input_manager.h"

#include "s_user_interface.h"

#include "at_atlas_handler.h"
#include "r_render_data.h"
#include "r_render_group.h"
#include "r_asset_shader.h"
#include "r_asset_texture.h"
#include "r_asset_dynamic_render_font.h"
#include "a_asset_loaded_sound.h"

#include "DEBUG_core.h"
#include "DEBUG_core.cpp"

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

#include "r_render_group.cpp"
#include "r_draw.cpp"

#if 0
#include "r_render_API.cpp"
#include "s_user_interface_core.cpp"
#endif

#if RENDERER_BACKEND == RENDERER_BACKEND_OPENGL
#include "r_opengl.h"
#include "r_opengl.cpp"
#elif RENDERER_BACKEND == RENDERER_BACKEND_VULKAN
#include "r_vulkan.h"
#include "r_vulkan.cpp"
#elif RENDERER_BACKEND == RENDERER_BACKEND_SDL_GPU
#include "r_sdl_gpu.h"
#include "r_sdl_gpu.cpp"
#elif RENDERER_BACKEND == RENDERER_BACKEND_HEADLESS
#include "r_headless_renderer.h"
#include "r_headless_renderer.cpp"
#else
#error Invalid backend...
#endif


#if PROFILER_ENABLED
#define GAME_UPDATE_AND_RENDER(name) void name(global_context_t *context, render_state_t *render_state, audio_manager_t *audio_manager, asset_manager_t *asset_manager, input_manager_t *input_manager, float32 frame_time, DEBUG_state_data_t *DEBUG_global_state_in) 
#else
#define GAME_UPDATE_AND_RENDER(name) void name(global_context_t *context, render_state_t *render_state, audio_manager_t *audio_manager, asset_manager_t *asset_manager, input_manager_t *input_manager, float32 frame_time) 
#endif

typedef GAME_UPDATE_AND_RENDER(game_update_and_render_t);

#endif
