#if !defined(S_AUDIO_MANAGER_H)
/* ========================================================================
   $File: s_audio_manager.h $
   $Date: Sun, 24 Aug 25: 10:50AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define S_AUDIO_MANAGER_H
#include <SDL3/SDL_audio.h>

#include "c_base.h"
#include "c_types.h"
#include "c_math.h"
#include "c_log_assert.h"
#include "c_memory_arena.h"
#include "c_string.h"
#include "c_array.h"
#include "c_file_api.h"

#include "a_asset_loaded_sound.h"

typedef struct audio_device
{
    u32           device_id;
    s32           device_buffer_size_in_sample_frames;
    s32           device_buffer_size_ms;

    u32           device_frequency;
    u32           channel_count;

    SDL_AudioSpec device_spec;
}audio_device_t;

typedef struct audio_buffer
{
    s16 *sample_buffer;
    s32  sample_count;

    s32  bytes_per_sample;
}audio_buffer_t;

typedef struct audio_manager
{
    // NOTE(Sleepster): This is the child of whatever arena you wish
    memory_arena_t   playing_sound_arena;
    
    audio_device_t   current_playback_device;
    SDL_AudioFormat  engine_audio_format;

    SDL_AudioSpec    audio_manager_spec;

    SDL_AudioStream *stream;
    audio_buffer_t   buffer;

    playing_sound_t *first_playing_sound;
    playing_sound_t *first_free_playing_sound;
}audio_manager_t;

/*===========================================
  =============== GENERAL API ===============
  ===========================================*/
internal void s_audio_manager_init(audio_manager_t *audio_manager);
internal void s_audio_manager_fill_sound_buffer(asset_manager_t *asset_manager, audio_manager_t *audio_manager, float32 delta_time);

#endif
