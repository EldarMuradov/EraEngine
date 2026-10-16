// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"
#include "core/math.h"

#define C_HZ		261.63f
#define C_SHARP_HZ	277.18f
#define D_HZ		293.66f
#define D_SHARP_HZ	311.13f
#define E_HZ		329.63f
#define F_HZ		349.23f
#define F_SHARP_HZ	369.99f
#define G_HZ		392.00f
#define G_SHARP_HZ	415.30f
#define A_HZ		440.00f
#define A_SHARP_HZ	466.16f
#define B_HZ		493.88f

#define MAX_SYNTH_SIZE 1024

namespace era_engine
{
	struct ERA_CORE_API AudioSynth
	{
		NODISCARD virtual uint32 get_samples(float* buffer, uint32 num_samples) = 0;
		NODISCARD virtual float get_duration() const { return 0.f; } // You only need to override this, if you don't stream the audio.
	};

	struct ERA_CORE_API SineSynth : AudioSynth
	{
		SineSynth(float duration = 10.f, float hz = C_HZ)
			: AudioSynth()
		{
			this->hz = hz;

			// Round duration such that there is an integer amount of waves over the runtime. This prevents clicking for looping sounds.
			float waves = duration * hz;
			duration = floor(waves) / hz;

			this->duration = duration;
			this->total_num_samples = (uint32)(duration * num_channels * sample_hz);
		}

		NODISCARD virtual uint32 get_samples(float* buffer, uint32 num_samples) override
		{
			if (offset + num_samples > total_num_samples)
			{
				num_samples = total_num_samples - offset;
			}

			float factor = M_TAU / sample_hz * hz;
			for (uint32 i = 0; i < num_samples; ++i, ++offset)
			{
				buffer[i] = sin(offset * factor);
			}
			return num_samples;
		}

		NODISCARD virtual float get_duration() const override { return duration; }

		static const uint32 num_channels = 1;
		static const uint32 sample_hz = 44100;

	private:
		uint32 total_num_samples;
		float hz;
		float duration;

		uint32 offset = 0;
	};
}