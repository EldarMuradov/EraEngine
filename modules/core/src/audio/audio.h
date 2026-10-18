// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "audio/sound.h"
#include "audio/sound_management.h"
#include "audio/reverb.h"

#include "core/math.h"

#include <xaudio2.h>

namespace era_engine
{
    struct ERA_CORE_API MasterAudioSettings
    {
        float volume = 0.1f;
        bool reverb_enabled = true;
        ReverbPreset reverb_preset = REVERB_PRESET_DEFAULT;
    };

    extern MasterAudioSettings master_audio_settings;
    extern float sound_type_volumes[SOUND_TYPE_COUNT];

    bool initialize_audio();
    void shutdown_audio();

    void set_audio_listener(vec3 position, quat rotation, vec3 velocity = vec3(0.f));

    void update_audio(float dt);

    struct ERA_CORE_API SoundHandle
    {
        operator bool() { return id != 0; }
        uint32 id;
    };

    SoundHandle play_2d_sound(const SoundID& id, const SoundSettings& settings);
    SoundHandle play_3d_sound(const SoundID& id, vec3 position, const SoundSettings& settings);

    void restart_all_sounds();

    bool sound_still_playing(SoundHandle handle);
    bool stop(SoundHandle handle, float fade_out_time = 0.1f);

    // Only hold on to this pointer for one frame! Retrieve each frame! Returns null, if sound has stopped already.
    SoundSettings* get_settings(SoundHandle handle);

    float db_to_volume(float db);
    float volume_to_db(float volume);
}