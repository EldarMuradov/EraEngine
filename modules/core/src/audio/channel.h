// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "audio/sound.h"

#include "core/math.h"

#include <x3daudio.h>

namespace era_engine
{
    enum ChannelState
    {
        CHANNEL_STATE_TO_PLAY,
        CHANNEL_STATE_PLAYING,

        CHANNEL_STATE_STOPPING,
        CHANNEL_STATE_STOPPED,

        CHANNEL_STATE_VIRTUALIZING,
        CHANNEL_STATE_VIRTUAL,
    };

    struct ERA_CORE_API PropertyFader
    {
        void initialize(float start)
        {
            current = start;
            total_time = 0.f;
        }

        void start_fade(float to, float time)
        {
            this->from = current;
            this->to = to;
            this->total_time = time;
            this->timer = 0.f;

            if (time <= 0.f)
            {
                this->current = to;
            }
        }

        void update(float dt)
        {
            if (total_time != 0.f)
            {
                timer += dt;
                current = lerp(from, to, clamp01(timer / total_time));
            }
        }

        float from, to;
        float total_time;
        float timer;

        float current;
    };

    struct ERA_CORE_API AudioContext
    {
        com<IXAudio2> xaudio;

        IXAudio2MasteringVoice* master_voice;
        XAUDIO2_VOICE_DETAILS master_voice_details;

        IXAudio2SubmixVoice* sound_type_submix_voices[SOUND_TYPE_COUNT];
        XAUDIO2_VOICE_DETAILS sound_type_submix_voice_details[SOUND_TYPE_COUNT];
        IXAudio2SubmixVoice* reverb_submix_voices[SOUND_TYPE_COUNT];

        X3DAUDIO_HANDLE xaudio_3d;
        X3DAUDIO_LISTENER listener;
    };

    struct ERA_CORE_API AudioChannel
    {
        AudioChannel(const AudioContext& context, const ref<AudioSound>& _sound, const SoundSettings& settings);
        AudioChannel(const AudioContext& context, const ref<AudioSound>& _sound, vec3 position, const SoundSettings& settings);
        ~AudioChannel();

        void update(const AudioContext& context, float dt);
        void stop(float fade_out_time);

        SoundSettings* get_settings() { return &user_settings; }

        bool has_stopped();

        ref<AudioSound> sound;
        bool positioned;
        vec3 position;

    private:
        void initialize(const AudioContext& context, const ref<AudioSound>& sound, const SoundSettings& settings, bool positioned, vec3 position = vec3(0.f));

        void update_sound_settings(const AudioContext& context, float dt);

        bool should_be_virtual();

        uint32 update_3d_timer = 0;

        volatile ChannelState state = CHANNEL_STATE_TO_PLAY;

        PropertyFader up_down_fader;
        PropertyFader volume_fader;
        PropertyFader pitch_fader;

        IXAudio2SourceVoice* voice;

        uint32 src_channels;

        SoundSettings user_settings;
        SoundSettings old_user_settings;

        float old_volume = -1.f;
        float old_pitch = -1.f;

        struct voice_callback : IXAudio2VoiceCallback
        {
            AudioChannel* channel;

            virtual void __stdcall OnVoiceProcessingPassStart(uint32 bytesRequired) override {}
            virtual void __stdcall OnVoiceProcessingPassEnd() override {}
            virtual void __stdcall OnStreamEnd() override
            { /*std::cout << "Stream end\n";*/
                channel->stop(0.f);
            }
            virtual void __stdcall OnVoiceError(void* bufferContext, HRESULT error) override { std::cerr << "Error!\n"; }
            virtual void __stdcall OnBufferStart(void* bufferContext) override {}
            virtual void __stdcall OnBufferEnd(void* bufferContext) override
            { /*std::cout << "Buffer end\n";*/
                if (bufferContext)
                {
                    ReleaseSemaphore((HANDLE)bufferContext, 1, 0);
                }
            }
            virtual void __stdcall OnLoopEnd(void* bufferContext) override {}
        };

        voice_callback voice_callback;

        HANDLE thread_handle = INVALID_HANDLE_VALUE;

        HANDLE buffer_end_semaphore;

        bool thread_stopped = true;

        friend DWORD WINAPI stream_file_audio(void* parameter);
        friend DWORD WINAPI stream_synth_audio(void* parameter);
    };

}