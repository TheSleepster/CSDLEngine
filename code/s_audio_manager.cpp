/* ========================================================================
   $File: s_audio_manager.cpp $
   $Date: Sun, 24 Aug 25: 10:51AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "s_audio_manager.h"

internal void
DEBUG_create_sine_wave(s16 *buffer, s32 sample_count)
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

internal void
s_audio_manager_init(audio_manager_t *audio_manager)
{
    audio_manager->current_playback_device.device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, null);
    if(audio_manager->current_playback_device.device_id != 0)
    {
        audio_manager->audio_manager_spec.freq     = 48000;
        audio_manager->audio_manager_spec.format   = SDL_AUDIO_S16LE;
        audio_manager->audio_manager_spec.channels = 2;
                
        bool32 result = SDL_GetAudioDeviceFormat(audio_manager->current_playback_device.device_id,
                                                 &audio_manager->current_playback_device.device_spec,
                                                 &audio_manager->current_playback_device.device_buffer_size_in_sample_frames);
        if(result)
        {
            log_info("Device: '%s' opened. Device data format: '%d'. Device channel count: '%d', Device sample rate: '%d'...\n",
                     SDL_GetAudioDeviceName(audio_manager->current_playback_device.device_id),
                     audio_manager->current_playback_device.device_spec.format,
                     audio_manager->current_playback_device.device_spec.channels,
                     audio_manager->current_playback_device.device_spec.freq);

            s32 sample_rate        = audio_manager->current_playback_device.device_spec.freq;
            s32 device_buffer_size = audio_manager->current_playback_device.device_buffer_size_in_sample_frames;
            
            audio_manager->current_playback_device.device_buffer_size_ms = (s32)((((s64)device_buffer_size) * 1000) / sample_rate);
            audio_manager->stream = SDL_CreateAudioStream(&audio_manager->audio_manager_spec, &audio_manager->current_playback_device.device_spec);
            if(audio_manager->stream != null)
            {
                log_info("Successfully created a new SDL_AudioStream!\n");
                bool32 did_bind = SDL_BindAudioStream(audio_manager->current_playback_device.device_id, audio_manager->stream);
                if(did_bind)
                {
                    log_info("Bound the audio stream to the device: '%s'...\n",
                             SDL_GetAudioDeviceName(audio_manager->current_playback_device.device_id));

                    SDL_ResumeAudioDevice(audio_manager->current_playback_device.device_id);
                }
                else
                {
                    log_error("Failure to bind the audio stream to the primary device... error: '%s'...\n", SDL_GetError());
                }
            }
        }
        else
        {
            log_error("Failure to open the system's default audio device... error: '%s'\n", SDL_GetError());
        }
    }
        
    audio_manager->buffer.bytes_per_sample = 2.0f * sizeof(s16); 
}

internal void
s_audio_manager_handle_and_mix_all_playing_sounds(asset_manager_t *asset_manager,
                                                  audio_manager_t *audio_manager,
                                                  s32              samples_to_write,
                                                  float32          delta_time)
{
    const float32 master_volume = 0.1f;

    // NOTE(Sleepster): We mix the audio samples as 32 bit HD audio so we can prevent peaking. Then truncate to 16bit
    float32 *mixer_buffer00 = c_arena_push_array(&global_context.temporary_arena, float32, samples_to_write);
    float32 *mixer_buffer01 = c_arena_push_array(&global_context.temporary_arena, float32, samples_to_write);

    float32 *dest00 = mixer_buffer00;
    float32 *dest01 = mixer_buffer01;

    playing_sound_t **playing_sound_ptr = &audio_manager->first_playing_sound;
    while(*playing_sound_ptr)
    {
        bool8 is_finished_playing = false;

        playing_sound_t *sound = *playing_sound_ptr;
        if(!sound->is_paused)
        {
            u32 total_samples_to_mix = samples_to_write;
            loaded_sound_t *sound_data = sound->sound_handle.sound;
            if(sound_data)
            {
                if(sound->next_sound_handle.is_valid && sound->next_sound_handle.sound != sound->sound_handle.sound)
                {
                    s_asset_loaded_sound_create(asset_manager, &sound->next_sound_handle); 
                }

                u32 mixing_count               = total_samples_to_mix;
                u32 remaining_samples_in_sound = (s32)(floorf(sound_data->sample_count - (s32)floor(sound->play_cursor)) / sound->pitch_shift);
                mixing_count                   = Min(mixing_count, remaining_samples_in_sound);

                real32 running_sample_index = sound->play_cursor;
                for(u32 sample_index = 0;
                    sample_index < mixing_count;
                    ++sample_index)
                {
                    s32 floored_index = (s32)running_sample_index;
                    s32 sample_offset = floored_index % sound_data->sample_count;

                    real32 sample_value00 = (float32)(sound_data->sample_data[sample_offset * 2]);
                    real32 sample_value01 = (float32)(sound_data->sample_data[sample_offset * 2 + 1]);

                    *dest00++ += sample_value00 * sound->current_playing_volume.elements[0];
                    *dest01++ += sample_value01 * sound->current_playing_volume.elements[1];

                    running_sample_index += sound->pitch_shift;
                }

                vec2_approach(&sound->current_playing_volume,
                               sound->target_playing_volume,
                               sound->d_volumet,
                               delta_time);
                
                sound->play_cursor    = running_sample_index;
                total_samples_to_mix -= mixing_count;
                if(sound->play_cursor >= sound_data->sample_count)
                {
                    if(sound->next_sound_handle.is_valid)
                    {
                        sound->sound_handle = sound->next_sound_handle;
                        sound->play_cursor  = 0.0f;
                    }
                    else
                    {
                        is_finished_playing = true;
                    }
                }
            }
            else
            {
                s_asset_loaded_sound_create(asset_manager, &sound->sound_handle); 
            }
        }

        if(is_finished_playing)
        {
            *playing_sound_ptr = sound->next;
            sound->next        = audio_manager->first_free_playing_sound;

            audio_manager->first_free_playing_sound = sound;
        }
        else
        {
            playing_sound_ptr = &sound->next;
        }
    }

    dest00 = mixer_buffer00;
    dest01 = mixer_buffer01;

    s16 *buffer_data = audio_manager->buffer.sample_buffer;
    for(s32 sample_index = 0;
        sample_index < samples_to_write;
        ++sample_index)
    {
        *buffer_data++ = (s16)((*dest00++ * master_volume) + 0.5f);
        *buffer_data++ = (s16)((*dest01++ * master_volume) + 0.5f);
    }
}

internal void
s_audio_manager_fill_sound_buffer(asset_manager_t *asset_manager, audio_manager_t *audio_manager, float32 delta_time)
{
    s32 sample_rate      = audio_manager->audio_manager_spec.freq;
    s32 bytes_per_sample = audio_manager->audio_manager_spec.channels * sizeof(s16);
            
    float32 device_latency = audio_manager->current_playback_device.device_buffer_size_ms + 2;

    s32 samples_to_write = (s32)(ceilf(((float32)device_latency * (float32)sample_rate) / 1000.0f));
    s32 bytes_to_write   = samples_to_write * bytes_per_sample;
            
    s32 queued_audio = SDL_GetAudioStreamQueued(audio_manager->stream);
    if(queued_audio < bytes_to_write)
    {
        audio_manager->buffer.sample_buffer = (s16*)c_arena_push_size(&global_context.temporary_arena, bytes_to_write);
        if(audio_manager->buffer.sample_buffer)
        {
            //DEBUG_create_sine_wave(audio_manager->buffer.sample_buffer, samples_to_write);
            s_audio_manager_handle_and_mix_all_playing_sounds(asset_manager, audio_manager, samples_to_write, delta_time);

            bool32 result = SDL_PutAudioStreamData(audio_manager->stream,
                                                   audio_manager->buffer.sample_buffer,
                                                   bytes_to_write);
            if(!result)
            {
                log_error("Could not put SDL_AudioStream data... Error: %s'\n", SDL_GetError());
            }
        }
    }
}
