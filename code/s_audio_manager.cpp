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
s_audio_manager_fill_sound_buffer(audio_manager_t *audio_manager)
{
    s32 sample_rate      = audio_manager->audio_manager_spec.freq;
    s32 bytes_per_sample = audio_manager->audio_manager_spec.channels * sizeof(s16);
            
    float32 device_latency = audio_manager->current_playback_device.device_buffer_size_ms;

    s32 samples_to_write = (s32)(ceilf(((float32)device_latency * (float32)sample_rate) / 1000.0f));
    s32 bytes_to_write   = samples_to_write * bytes_per_sample;
            
    s32 queued_audio = SDL_GetAudioStreamQueued(audio_manager->stream);
    if(queued_audio < bytes_to_write)
    {
        audio_manager->buffer.sample_buffer = (s16*)c_arena_push_size(&global_context.temporary_arena,
                                                                     bytes_to_write);
        if(audio_manager->buffer.sample_buffer)
        {
            DEBUG_create_sine_wave(audio_manager->buffer.sample_buffer, samples_to_write);
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
