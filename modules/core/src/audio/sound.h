// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "audio/synth.h"
#include "core/string.h"
#include "asset/asset.h"

#include <xaudio2.h>
#include <functional>

#define SOUND_ID(id) SoundID{ id, COMPILE_TIME_STRING_HASH_64(id) }

namespace era_engine
{
    struct ERA_CORE_API SoundID
    {
        const char* id;
        uint64 hash;
    };

    enum SoundType
    {
        SOUND_TYPE_MUSIC,
        SOUND_TYPE_SFX,
        SOUND_TYPE_COUNT,
    };

    static const char* sound_type_names[] =
    {
        "Music",
        "Effects",
    };

    struct ERA_CORE_API SoundSpec
    {
        std::string name;
        AssetHandle asset;
        SoundType type;
        bool stream;
    };

    struct ERA_CORE_API AudioSound
    {
        virtual ~AudioSound();

        virtual AudioSynth* create_synth(void* buffer) const { return 0; }

        SoundID id;

        fs::path path;
        bool stream;
        bool is_synth;

        HANDLE file_handle = INVALID_HANDLE_VALUE;
        WAVEFORMATEXTENSIBLE wfx = {};
        uint32 chunk_size;
        uint32 chunk_position;
        BYTE* data_buffer = 0;

        SoundType type;
    };

    struct ERA_CORE_API SoundSettings
    {
        float volume = 1.f;
        float pitch = 1.f;
        float radius = 30.f;
        bool loop = false;

        float volume_fade_time = 0.1f;
        float pitch_fade_time = 0.1f;
    };

    NODISCARD ref<AudioSound> get_sound(SoundID id);

    void unload_sound(SoundID id);
    void unload_all_sounds();

    bool load_file_sound(SoundID id);

    template <typename Synth_, typename... Args_>
    inline bool load_synth_sound(const char* id_str, SoundType type, bool stream, const Args_&... a)
    {
        static_assert(std::is_base_of_v<AudioSynth, Synth_>, "Synthesizer must inherit from AudioSynth");
        static_assert(sizeof(Synth_) <= MAX_SYNTH_SIZE);

        SoundID id = { id_str, hash_string64(id_str) };
        SoundSpec spec = { {}, type, stream };

        bool check_for_existing_synth_sound(SoundID);
        void register_sound(SoundID, const ref<AudioSound>&);

        if (check_for_existing_synth_sound(id))
            return true;
        else
        {
            struct SynthSound : AudioSound
            {
                std::function<AudioSynth* (void*)> createFunc;

                virtual AudioSynth* create_synth(void* buffer) const override
                {
                    return createFunc(buffer);
                }
            };

            ref<AudioSound> sound;

            WAVEFORMATEX wfx = {};
            wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            wfx.nChannels = Synth_::num_channels;
            wfx.nSamplesPerSec = Synth_::sample_hz;
            wfx.wBitsPerSample = Synth_::num_channels * (uint32)sizeof(float) * 8;
            wfx.nBlockAlign = Synth_::num_channels * (uint32)sizeof(float);
            wfx.nAvgBytesPerSec = Synth_::sample_hz * Synth_::num_channels * (uint32)sizeof(float);
            wfx.cbSize = 0; // Set to zero for PCM or IEEE float.

            BYTE* data_buffer = 0;
            uint32 dataSize = 0;

            if (!stream)
            {
                sound = make_ref<AudioSound>();

                Synth_ synth(a...);
                uint32 total_num_samples = (uint32)(synth.get_duration() * Synth_::num_channels * Synth_::sample_hz);
                uint32 size = sizeof(float) * total_num_samples;

                data_buffer = new BYTE[size];
                synth.get_samples((float*)data_buffer, total_num_samples);

                dataSize = size;
            }
            else
            {
                auto s = make_ref<SynthSound>();

                s->createFunc = [=](void* buffer)
                    {
                        return new(buffer) Synth_(a...);
                    };

                sound = s;
            }

            sound->id = id;
            sound->stream = stream;
            sound->wfx = { wfx };
            sound->data_buffer = data_buffer;
            sound->chunk_size = dataSize;
            sound->is_synth = true;
            sound->type = type;

            register_sound(id, sound);

            return true;
        }
    }

    NODISCARD bool is_sound_extension(const fs::path& extension);
    NODISCARD bool is_sound_extension(const std::string& extension);
}