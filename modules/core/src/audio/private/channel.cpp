// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "audio/channel.h"

#define MAX_BUFFER_COUNT 3
#define STREAMING_BUFFER_SIZE (1024 * 8 * 6)
#define UPDATE_3D_PERIOD 3 // > 0.
#define VIRTUALIZE_FADE_TIME 0.1f

namespace era_engine
{
    DWORD WINAPI stream_file_audio(void* parameter);
    DWORD WINAPI stream_synth_audio(void* parameter);

    AudioChannel::AudioChannel(const AudioContext& context, const ref<AudioSound>& _sound, const SoundSettings& settings)
    {
        initialize(context, _sound, settings, false);
    }

    AudioChannel::AudioChannel(const AudioContext& context, const ref<AudioSound>& _sound, vec3 position, const SoundSettings& settings)
    {
        initialize(context, _sound, settings, true, position);
    }

    void AudioChannel::initialize(const AudioContext& context, const ref<AudioSound>& sound, const SoundSettings& settings, bool positioned, vec3 position)
    {
        this->sound = sound;
        this->voice_callback.channel = this;

        user_settings = settings;

        old_user_settings.volume = -1.f;
        old_user_settings.pitch = -1.f;
        old_user_settings.loop = false;

        this->positioned = positioned;
        this->position = position;

        XAUDIO2_SEND_DESCRIPTOR send_descriptors[2];

        // Direct
        send_descriptors[0].Flags = XAUDIO2_SEND_USEFILTER;
        send_descriptors[0].pOutputVoice = context.sound_type_submix_voices[sound->type];

        // Reverb
        send_descriptors[1].Flags = XAUDIO2_SEND_USEFILTER;
        send_descriptors[1].pOutputVoice = context.reverb_submix_voices[sound->type];

        // Reverb only for positioned voices
        const XAUDIO2_VOICE_SENDS send_list = {positioned ? 2u : 1u, send_descriptors};

        checkResult(context.xaudio->CreateSourceVoice(&voice, (WAVEFORMATEX*)&sound->wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &voice_callback, &send_list));
        checkResult(voice->Start());

        XAUDIO2_VOICE_DETAILS voice_details;
        voice->GetVoiceDetails(&voice_details);
        src_channels = voice_details.InputChannels;

        volume_fader.initialize(settings.volume);
        pitch_fader.initialize(settings.pitch);

        up_down_fader.initialize(1.f);

        update_sound_settings(context, 0.f);

        if (sound->stream)
        {
            thread_stopped = false;
            buffer_end_semaphore = CreateSemaphore(0, 0, MAX_BUFFER_COUNT, 0);

            auto func = (sound->is_synth) ? stream_synth_audio : stream_file_audio;
            thread_handle = CreateThread(0, 0, func, this, 0, 0);
        }
        else
        {
            XAUDIO2_BUFFER buffer = {0};
            buffer.AudioBytes = sound->chunk_size;
            buffer.pAudioData = sound->data_buffer;
            if (settings.loop)
            {
                buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
            }
            else
            {
                buffer.Flags = XAUDIO2_END_OF_STREAM;
            }

            checkResult(voice->SubmitSourceBuffer(&buffer));
        }
    }

    AudioChannel::~AudioChannel()
    {
        voice->DestroyVoice();
        if (sound->stream)
        {
            CloseHandle(buffer_end_semaphore);
            CloseHandle(thread_handle);
        }
    }

    void AudioChannel::update(const AudioContext& context, float dt)
    {
        switch (state)
        {
            case CHANNEL_STATE_TO_PLAY:
            {
                state = CHANNEL_STATE_PLAYING;
                if (should_be_virtual())
                {
                    state = CHANNEL_STATE_VIRTUAL;
                }
            }
            break;

            case CHANNEL_STATE_PLAYING:
            {
                update_sound_settings(context, dt);
                if (should_be_virtual())
                {
                    state = CHANNEL_STATE_VIRTUALIZING;
                    up_down_fader.start_fade(0.f, VIRTUALIZE_FADE_TIME);
                }
            }
            break;

            case CHANNEL_STATE_STOPPING:
            {
                update_sound_settings(context, dt);
                if (up_down_fader.current <= 0.f)
                {
                    voice->Stop();
                    state = CHANNEL_STATE_STOPPED;

                    if (sound->stream)
                    {
                        ReleaseSemaphore(buffer_end_semaphore, 1, 0);
                    }
                }
            }
            break;

            case CHANNEL_STATE_STOPPED:
            {
            }
            break;

            case CHANNEL_STATE_VIRTUALIZING:
            {
                update_sound_settings(context, dt);

                if (!should_be_virtual())
                {
                    up_down_fader.start_fade(1.f, VIRTUALIZE_FADE_TIME);
                    state = CHANNEL_STATE_PLAYING;
                }
                if (up_down_fader.current <= 0.f)
                {
                    state = CHANNEL_STATE_VIRTUAL;
                }
            }
            break;

            case CHANNEL_STATE_VIRTUAL:
            {
                if (!should_be_virtual())
                {
                    up_down_fader.start_fade(1.f, VIRTUALIZE_FADE_TIME);
                    state = CHANNEL_STATE_PLAYING;
                }
            }
            break;

            default:
                break;
        }
    }

    void AudioChannel::stop(float fade_out_time)
    {
        if (state != CHANNEL_STATE_STOPPING && state != CHANNEL_STATE_STOPPED)
        {
            state = CHANNEL_STATE_STOPPING;
            up_down_fader.start_fade(0.f, fade_out_time);
        }
    }

    bool AudioChannel::has_stopped()
    {
        return state == CHANNEL_STATE_STOPPED && thread_stopped;
    }

    void AudioChannel::update_sound_settings(const AudioContext& context, float dt)
    {
        user_settings.pitch = clamp(user_settings.pitch, 0.f, XAUDIO2_DEFAULT_FREQ_RATIO);

        if (user_settings.volume != old_user_settings.volume)
        {
            volume_fader.start_fade(user_settings.volume, user_settings.volume_fade_time);
        }
        if (user_settings.pitch != old_user_settings.pitch)
        {
            pitch_fader.start_fade(user_settings.pitch, user_settings.pitch_fade_time);
        }

        old_user_settings = user_settings;

        up_down_fader.update(dt);
        volume_fader.update(dt);
        pitch_fader.update(dt);

        float v = volume_fader.current * up_down_fader.current;
        if (v != old_volume)
        {
            voice->SetVolume(v);
            old_volume = v;
        }

        float p = pitch_fader.current;
        if (p != old_pitch)
        {
            voice->SetFrequencyRatio(p);
            old_pitch = p;
        }

        if (positioned)
        {
            if (update_3d_timer == 0)
            {
                quat rotation = quat::identity; // TODO

                vec3 forward = rotation * vec3(0.f, 0.f, -1.f);
                vec3 up = rotation * vec3(0.f, 1.f, 0.f);

                float channel_azimuths[8] = {};

                X3DAUDIO_EMITTER emitter = {0};
                emitter.ChannelCount = src_channels;
                emitter.ChannelRadius = 0.1f;
                emitter.InnerRadius = 2.f;
                emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.f;
                emitter.pChannelAzimuths = channel_azimuths;

#if 1
                emitter.CurveDistanceScaler = user_settings.radius;
                emitter.pVolumeCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
                emitter.pReverbCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
#else
                emitter.CurveDistanceScaler = 1;
#endif

                emitter.OrientFront = {forward.x, forward.y, -forward.z};
                emitter.OrientTop = {up.x, up.y, -up.z};
                emitter.Position = {position.x, position.y, -position.z};

                float matrix[8] = {};

                uint32 dst_channels = context.sound_type_submix_voice_details[sound->type].InputChannels;

                X3DAUDIO_DSP_SETTINGS dsp_settings = {0};
                dsp_settings.SrcChannelCount = src_channels;
                dsp_settings.DstChannelCount = dst_channels;
                dsp_settings.pMatrixCoefficients = matrix;

                UINT32 flags = 0;
                flags |= X3DAUDIO_CALCULATE_MATRIX;
                flags |= X3DAUDIO_CALCULATE_LPF_DIRECT;
                flags |= X3DAUDIO_CALCULATE_REVERB;
                flags |= X3DAUDIO_CALCULATE_LPF_REVERB;
                // flags |= X3DAUDIO_CALCULATE_DOPPLER;

                X3DAudioCalculate(context.xaudio_3d, &context.listener, &emitter, flags, &dsp_settings);

                checkResult(voice->SetOutputMatrix(context.sound_type_submix_voices[sound->type], src_channels, dst_channels, matrix));

                for (uint32 i = 0; i < src_channels; ++i)
                {
                    matrix[i] = dsp_settings.ReverbLevel;
                }

                checkResult(voice->SetOutputMatrix(context.reverb_submix_voices[sound->type], src_channels, 1, matrix));

                XAUDIO2_FILTER_PARAMETERS filter_parameters_direct = {LowPassFilter, 2.f * sin(X3DAUDIO_PI / 6.f * dsp_settings.LPFDirectCoefficient), 1.f};
                checkResult(voice->SetOutputFilterParameters(context.sound_type_submix_voices[sound->type], &filter_parameters_direct));

                XAUDIO2_FILTER_PARAMETERS filter_parameters_reverb = {LowPassFilter, 2.f * sin(X3DAUDIO_PI / 6.f * dsp_settings.LPFReverbCoefficient), 1.f};
                checkResult(voice->SetOutputFilterParameters(context.reverb_submix_voices[sound->type], &filter_parameters_reverb));

                // std::cout << dspSettings.ReverbLevel << ", " << dspSettings.LPFDirectCoefficient << ", " << dspSettings.LPFReverbCoefficient << '\n';

                update_3d_timer = UPDATE_3D_PERIOD;
            }

            --update_3d_timer;
        }
    }

    bool AudioChannel::should_be_virtual()
    {
        return false;
    }

    static DWORD WINAPI stream_file_audio(void* parameter)
    {
        AudioChannel* channel = (AudioChannel*)parameter;
        auto& sound = channel->sound;
        auto voice = channel->voice;

        BYTE buffers[MAX_BUFFER_COUNT][STREAMING_BUFFER_SIZE];

        uint32 current_buffer_index = 0;

        bool quit = false;

        while (!quit)
        {
            uint32 current_position = 0;

            if (SetFilePointer(sound->file_handle, sound->chunk_position, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
                break;

            while (!quit && current_position < sound->chunk_size)
            {
                DWORD size = STREAMING_BUFFER_SIZE;
                if (ReadFile(sound->file_handle, buffers[current_buffer_index], size, &size, 0) == 0)
                {
                    quit = true;
                    break;
                }

                current_position += size;

                XAUDIO2_BUFFER buffer = {0};
                buffer.AudioBytes = size;
                buffer.pAudioData = buffers[current_buffer_index];
                buffer.pContext = channel->buffer_end_semaphore;

                checkResult(voice->SubmitSourceBuffer(&buffer));

                XAUDIO2_VOICE_STATE state;
                voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
                while (!quit && state.BuffersQueued > MAX_BUFFER_COUNT - 1)
                {
                    WaitForSingleObject(channel->buffer_end_semaphore, INFINITE);
                    voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

                    if (channel->state == CHANNEL_STATE_STOPPED)
                    {
                        quit = true;
                    }
                }
                current_buffer_index++;
                current_buffer_index %= MAX_BUFFER_COUNT;
            }

            if (!channel->user_settings.loop)
                break;
        }

        if (!quit)
        {
            XAUDIO2_VOICE_STATE state;
            while (voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED), state.BuffersQueued > 0)
            {
                WaitForSingleObject(channel->buffer_end_semaphore, INFINITE);
            }
        }

        channel->stop(0.f);

        channel->thread_stopped = true;

        return 0;
    }

    static DWORD WINAPI stream_synth_audio(void* parameter)
    {
        AudioChannel* channel = (AudioChannel*)parameter;
        auto& sound = channel->sound;
        auto voice = channel->voice;

        float buffers[MAX_BUFFER_COUNT][STREAMING_BUFFER_SIZE];

        uint32 current_buffer_index = 0;

        bool quit = false;

        while (!quit)
        {
            uint8 synth_buffer[MAX_SYNTH_SIZE];
            AudioSynth* synth = sound->create_synth(synth_buffer);

            while (true)
            {
                uint32 size = STREAMING_BUFFER_SIZE;
                size = synth->get_samples(buffers[current_buffer_index], size);

                if (size == 0)
                    break;

                XAUDIO2_BUFFER buffer = {0};
                buffer.AudioBytes = size * sizeof(float);
                buffer.pAudioData = (BYTE*)buffers[current_buffer_index];
                buffer.pContext = channel->buffer_end_semaphore;

                checkResult(voice->SubmitSourceBuffer(&buffer));

                XAUDIO2_VOICE_STATE state;
                voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
                while (!quit && state.BuffersQueued > MAX_BUFFER_COUNT - 1)
                {
                    WaitForSingleObject(channel->buffer_end_semaphore, INFINITE);
                    voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);

                    if (channel->state == CHANNEL_STATE_STOPPED)
                    {
                        quit = true;
                    }
                }
                current_buffer_index++;
                current_buffer_index %= MAX_BUFFER_COUNT;
            }

            if (!channel->user_settings.loop)
                break;
        }

        if (!quit)
        {
            XAUDIO2_VOICE_STATE state;
            while (voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED), state.BuffersQueued > 0)
            {
                WaitForSingleObject(channel->buffer_end_semaphore, INFINITE);
            }
        }

        channel->stop(0.f);

        channel->thread_stopped = true;

        return 0;
    }
}