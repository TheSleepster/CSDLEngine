/* ========================================================================
   $File: s_audio_manager.cpp $
   $Date: Sun, 24 Aug 25: 10:51AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "s_audio_manager.h"

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
        
    audio_manager->buffer.samples_to_write = 12000;
    audio_manager->buffer.bytes_per_sample = 2.0f * sizeof(s16); 
    audio_manager->buffer.bytes_to_write   = audio_manager->buffer.samples_to_write * audio_manager->buffer.bytes_per_sample;
    audio_manager->buffer.sample_count     = audio_manager->buffer.bytes_to_write / sizeof(s16);
}
