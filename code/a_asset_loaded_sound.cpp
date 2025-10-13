/* ========================================================================
   $File: a_asset_loaded_sound.cpp $
   $Date: Mon, 25 Aug 25: 07:20AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "a_asset_loaded_sound.h"

internal inline WAVE_file_iterator_t
parse_WAVE_chunk_at(void *at, void *end)
{
    WAVE_file_iterator_t result;
    result.at  = (u8*)at;
    result.end = (u8*)end;

    return(result);
}

internal inline bool8
is_WAVE_chunk_valid(WAVE_file_iterator_t iterator)
{
    bool8 result = (iterator.at < iterator.end);
    return(result);
}

internal inline WAVE_file_iterator_t
get_next_WAVE_chunk(WAVE_file_iterator_t iterator)
{
    WAVE_chunk_t *chunk = (WAVE_chunk_t*)iterator.at;
    u32 chunk_size = (chunk->size + 1) & ~1;

    iterator.at += sizeof(WAVE_chunk_t) + chunk_size;

    return(iterator);
}

internal inline s32
get_WAVE_chunk_data_size(WAVE_file_iterator_t iterator)
{
    WAVE_chunk_t *chunk = (WAVE_chunk_t*)iterator.at;
    return(chunk->size);
}

internal inline void*
get_WAVE_chunk_data(WAVE_file_iterator_t iterator)
{
    void *result = iterator.at + sizeof(WAVE_chunk_t);
    return(result);
}

internal u32
get_WAVE_chunk_type(WAVE_file_iterator_t iterator)
{
    WAVE_chunk_t *chunk = (WAVE_chunk_t*)iterator.at;
    u32 result = chunk->ID;

    return(result);
}

internal loaded_sound_t 
s_asset_load_WAV_file(asset_manager_t *asset_manager, string_t filename, string_t filedata)
{
    DEBUG_TIMED_BLOCK();

    loaded_sound_t result = {};
    result.filedata = filedata;
    
    WAVE_header_t *file_header = (WAVE_header_t*)filedata.data;
    Assert(file_header->RIFFID == WAVE_chunkID_RIFF);
    Assert(file_header->WAVEID == WAVE_chunkID_WAVE);

    u32  channel_count    = 0;
    u32  sample_data_size = 0;
    s16 *sample_data      = null;
    for(WAVE_file_iterator_t iterator = parse_WAVE_chunk_at(file_header + 1, (u8*)(file_header + 1) + file_header->Size - 4);
        is_WAVE_chunk_valid(iterator);
        iterator = get_next_WAVE_chunk(iterator))
    {
        switch(get_WAVE_chunk_type(iterator))
        {
            case WAVE_chunkID_fmt:
            {
                WAVE_format_data_t *format = (WAVE_format_data_t*)get_WAVE_chunk_data(iterator);

                // NOTE(Sleepster): Only raw PCM data at 48000hz on stereo is supported.
                Assert(format->wFormatTag     == 1);
                Assert(format->nSamplesPerSec == 48000);
                Assert(format->nBlockAlign    == (2 * format->nChannels));

                channel_count = format->nChannels;
            }break;
            case WAVE_chunkID_data:
            {
                sample_data      = (s16 *)get_WAVE_chunk_data(iterator);
                sample_data_size = get_WAVE_chunk_data_size(iterator);
            }break;
        }
    }
    Assert(channel_count && sample_data);
    if(channel_count == 1 || channel_count == 2)
    {
        result.channel_count = channel_count;
        result.sample_count  = ((sample_data_size / sizeof(u8)) + 1) & ~1;
        result.sample_data   = sample_data;
    }
    else
    {
        log_warning("WAVE file data not loaded... file: '%s' is not of a supported format. Channel count: '%d'...\n",
                    C_STR(filename), channel_count);
    }

    return(result);
}

internal void
s_asset_loaded_sound_create(asset_manager_t *asset_manager, asset_handle_t *handle)
{
    Assert(handle->type == AT_SOUND);
    asset_slot_t *slot_data = handle->asset_slot;
    if(slot_data->slot_state == ASS_UNLOADED || slot_data->slot_state == ASS_RELOADING)
    {
        s_asset_load_data_from_asset_file_or_path(asset_manager,
                                                 &handle->asset_slot->loaded_sound.filedata,
                                                  asset_manager->sound_catalog.sound_allocator,
                                                  slot_data,
                                                  ZA_TAG_CACHE);
    }
    if(c_string_is_valid(slot_data->loaded_sound.filedata))
    {
        slot_data->loaded_sound = s_asset_load_WAV_file(asset_manager, slot_data->filename, slot_data->loaded_sound.filedata);
    }
}

internal asset_handle_t
s_asset_loaded_sound_get(asset_manager_t *asset_manager, string_t name)
{
    asset_handle_t result = {};

    asset_slot_t *valid_slot = (asset_slot_t*)c_hash_get_value(&asset_manager->sound_catalog.sound_hash, name);
    if(valid_slot)
    {
        result.is_valid   = true;
        result.type       = AT_SOUND;
        result.asset_slot = valid_slot;

        s_asset_loaded_sound_create(asset_manager, &result);
        result.sound = &valid_slot->loaded_sound;
    }
    else
    {
        log_warning("Invalid sound file name: '%s'. Could not find an audio file with that name in the game package...\n", C_STR(name));
    }

    return(result);
}

/*==============================================
  =============== PLAYING SOUNDS ===============
  ==============================================*/
internal playing_sound*
s_asset_playing_sound_create(audio_manager_t *audio_manager,
                             asset_handle_t   sound_handle,
                             vec2_t           starting_volume)
{
    if(!audio_manager->first_free_playing_sound)
    {
        audio_manager->first_free_playing_sound       = c_arena_push_struct(&audio_manager->playing_sound_arena, playing_sound_t);
        audio_manager->first_free_playing_sound->next = null;
    }
    playing_sound_t *result = audio_manager->first_free_playing_sound;
    audio_manager->first_free_playing_sound = result->next;

    ZeroStruct(*result);
    result->sound_handle           = sound_handle;
    result->play_cursor            = 0.0f;
    result->current_playing_volume = vec2(1.0f, 1.0f);
    result->pitch_shift            = 1.0f;
    result->is_paused              = false;
    result->next                   = audio_manager->first_playing_sound;

    audio_manager->first_playing_sound = result;
    return(result);
}

internal inline void
s_asset_playing_sound_set_target_volume(playing_sound_t *sound, float32 target_x, float32 target_y, float32 fade_x, float32 fade_y)
{
    sound->target_playing_volume  = vec2(target_x, target_y);
    sound->d_volumet              = vec2(fade_x, fade_y);
}

internal inline void
s_asset_playing_sound_set_pitch(playing_sound_t *sound, float32 pitch)
{
    sound->pitch_shift = pitch;
}

internal inline void
a_playing_sound_pause(playing_sound_t *sound)
{
    sound->is_paused = true;
}

internal inline void
a_playing_sound_continue(playing_sound_t *sound)
{
    sound->is_paused = false;
}

internal inline void
a_playing_sound_set_volume(playing_sound_t *sound, float32 norm_volume_x, float32 norm_volume_y)
{
    sound->current_playing_volume = vec2(norm_volume_x, norm_volume_y);
}

internal inline void
a_playing_sound_set_looped(playing_sound_t *sound)
{
    sound->next_sound_handle = sound->sound_handle;
}

internal inline void
a_playing_sound_free(audio_manager_t *audio_manager, playing_sound_t *sound)
{
    playing_sound_t *last_free_playing_sound = audio_manager->first_free_playing_sound;

    sound       = audio_manager->first_free_playing_sound;
    sound->next = last_free_playing_sound;
}
