#if !defined(S_AUDIO_MANAGER_H)
/* ========================================================================
   $File: s_audio_manager.h $
   $Date: Sun, 24 Aug 25: 10:50AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_AUDIO_MANAGER_H
#include "c_base.h"
#include "c_types.h"
#include "c_math.h"
#include "c_debug.h"
#include "c_memory.h"
#include "c_string.h"
#include "c_array.h"
#include "c_file_api.h"

#include <SDL3/SDL_audio.h>

typedef struct audio_device
{
    u32 device_id;
    s32 audio_buffer_size;
    s32 audio_buffer_size_ms;

    u32 device_frequency;
    u32 channel_count;
}audio_device_t;

typedef struct audio_manager
{
    audio_device_t  current_playback_device;
    SDL_AudioFormat engine_audio_format;
}audio_manager_t;

#endif
