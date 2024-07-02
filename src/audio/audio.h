// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "audio/sound.h"
#include "audio/sound_management.h"
#include "audio/reverb.h"
#include "core/math.h"

#include <xaudio2.h>

namespace era_engine
{
	struct master_audio_settings
	{
		float volume = 0.1f;
		bool reverbEnabled = true;
		reverb_preset reverbPreset = reverb_preset_default;
	};

	extern master_audio_settings masterAudioSettings;
	extern float soundTypeVolumes[sound_type_count];

	bool initializeAudio();
	void shutdownAudio();

	void setAudioListener(vec3 position, quat rotation, vec3 velocity = vec3(0.f));

	void updateAudio(float dt);

	struct sound_handle
	{
		operator bool() { return id != 0; }
		uint32 id;
	};

	sound_handle play2DSound(const sound_id& id, const sound_settings& settings);
	sound_handle play3DSound(const sound_id& id, vec3 position, const sound_settings& settings);

	void restartAllSounds();

	bool soundStillPlaying(sound_handle handle);
	bool stop(sound_handle handle, float fadeOutTime = 0.1f);

	// Only hold on to this pointer for one frame! Retrieve each frame! Returns null, if sound has stopped already.
	NODISCARD sound_settings* getSettings(sound_handle handle);

	float dbToVolume(float db);
	float volumeToDB(float volume);
}