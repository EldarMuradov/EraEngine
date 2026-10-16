// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#define XAUDIO2_HELPER_FUNCTIONS
#include <xaudio2.h>
#include <xaudio2fx.h>

#undef M_PI

#include "audio/audio.h"
#include "audio/sound.h"
#include "audio/channel.h"

#include "core/cpu_profiling.h"

#include <x3daudio.h>

namespace era_engine
{
	MasterAudioSettings master_audio_settings;
	MasterAudioSettings old_master_audio_settings;

	static PropertyFader master_volume_fader;

	float sound_type_volumes[SOUND_TYPE_COUNT];
	static float old_sound_type_volumes[SOUND_TYPE_COUNT];
	static PropertyFader sound_type_volume_faders[SOUND_TYPE_COUNT];

	static AudioContext context;

	typedef std::unordered_map<uint32, ref<AudioChannel>> ChannelMap;

	static ChannelMap channels;
	static uint32 next_channel_id = 1;

	static XAUDIO2FX_REVERB_I3DL2_PARAMETERS reverb_presets[] =
	{
		XAUDIO2FX_I3DL2_PRESET_DEFAULT,
		XAUDIO2FX_I3DL2_PRESET_GENERIC,
		XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
		XAUDIO2FX_I3DL2_PRESET_ROOM,
		XAUDIO2FX_I3DL2_PRESET_BATHROOM,
		XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
		XAUDIO2FX_I3DL2_PRESET_STONEROOM,
		XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
		XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
		XAUDIO2FX_I3DL2_PRESET_CAVE,
		XAUDIO2FX_I3DL2_PRESET_ARENA,
		XAUDIO2FX_I3DL2_PRESET_HANGAR,
		XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
		XAUDIO2FX_I3DL2_PRESET_HALLWAY,
		XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
		XAUDIO2FX_I3DL2_PRESET_ALLEY,
		XAUDIO2FX_I3DL2_PRESET_FOREST,
		XAUDIO2FX_I3DL2_PRESET_CITY,
		XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
		XAUDIO2FX_I3DL2_PRESET_QUARRY,
		XAUDIO2FX_I3DL2_PRESET_PLAIN,
		XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
		XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
		XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
		XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
		XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
		XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
		XAUDIO2FX_I3DL2_PRESET_PLATE,
	};

	/*
		Our audio graph looks like this:

		[Source(s)] ----> [Sound type submix] ----> [Master]
			 `-> [Sound type reverb] -^
	*/

	static void set_reverb()
	{
		bool reverb_on = master_audio_settings.reverb_enabled && master_audio_settings.reverb_preset != REVERB_NONE;

		if (reverb_on)
		{
			XAUDIO2FX_REVERB_PARAMETERS reverb_parameters;
			ReverbConvertI3DL2ToNative(&reverb_presets[master_audio_settings.reverb_preset], &reverb_parameters);

			for (uint32 i = 0; i < SOUND_TYPE_COUNT; ++i)
			{
				context.reverb_submix_voices[i]->SetEffectParameters(0, &reverb_parameters, sizeof(reverb_parameters));
			}
		}

		for (uint32 i = 0; i < SOUND_TYPE_COUNT; ++i)
		{
			float level[16];
			float reverb_level = reverb_on ? 1.f : 0.f;
			for (uint32 j = 0; j < context.sound_type_submix_voice_details[i].InputChannels; ++j)
			{
				level[j] = reverb_level;
			}

			context.reverb_submix_voices[i]->SetOutputMatrix(context.sound_type_submix_voices[i], 1, context.sound_type_submix_voice_details[i].InputChannels, level);
		}
	}

	bool initialize_audio()
	{
		uint32 flags = 0;
#ifdef _DEBUG
		flags |= XAUDIO2_DEBUG_ENGINE;
#endif
		checkResult(XAudio2Create(context.xaudio.GetAddressOf(), flags));
		checkResult(context.xaudio->CreateMasteringVoice(&context.master_voice));

		master_volume_fader.initialize(master_audio_settings.volume);
		context.master_voice->SetVolume(master_audio_settings.volume);

		DWORD channel_mask;
		context.master_voice->GetChannelMask(&channel_mask);
		context.master_voice->GetVoiceDetails(&context.master_voice_details);

		X3DAudioInitialize(channel_mask, X3DAUDIO_SPEED_OF_SOUND, context.xaudio_3d);

		for (uint32 i = 0; i < SOUND_TYPE_COUNT; ++i)
		{
			sound_type_volumes[i] = old_sound_type_volumes[i] = 1.f;
			sound_type_volume_faders[i].initialize(sound_type_volumes[i]);

			checkResult(context.xaudio->CreateSubmixVoice(&context.sound_type_submix_voices[i], context.master_voice_details.InputChannels, context.master_voice_details.InputSampleRate, 0, 1));
			context.sound_type_submix_voices[i]->GetVoiceDetails(&context.sound_type_submix_voice_details[i]);
			context.sound_type_submix_voices[i]->SetVolume(sound_type_volumes[i]);

			IUnknown* reverb_xapo;
			checkResult(XAudio2CreateReverb(&reverb_xapo));

			XAUDIO2_EFFECT_DESCRIPTOR descriptor;
			descriptor.InitialState = true;
			descriptor.OutputChannels = 1;
			descriptor.pEffect = reverb_xapo;

			XAUDIO2_EFFECT_CHAIN chain;
			chain.EffectCount = 1;
			chain.pEffectDescriptors = &descriptor;

			XAUDIO2_SEND_DESCRIPTOR send_descriptor;
			send_descriptor.Flags = 0;
			send_descriptor.pOutputVoice = context.sound_type_submix_voices[i];

			const XAUDIO2_VOICE_SENDS send_list = { 1, &send_descriptor };

			checkResult(context.xaudio->CreateSubmixVoice(&context.reverb_submix_voices[i], 1, context.sound_type_submix_voice_details[i].InputSampleRate, 0, 0, &send_list, &chain));

			reverb_xapo->Release();
		}

		set_reverb();

		context.listener.OrientFront = { 0.f, 0.f, 1.f };
		context.listener.OrientTop = { 0.f, 1.f, 0.f };
		context.listener.pCone = (X3DAUDIO_CONE*)&X3DAudioDefault_DirectionalCone;

		load_sound_registry();

		return true;
	}

	void shutdown_audio()
	{
		channels.clear();

		if (context.xaudio)
		{
			context.xaudio->StopEngine();
			context.xaudio.Reset();
		}
	}

	void set_audio_listener(vec3 position, quat rotation, vec3 velocity)
	{
		vec3 forward = rotation * vec3(0.f, 0.f, -1.f);
		vec3 up = rotation * vec3(0.f, 1.f, 0.f);

		// XAudio2 uses a lefthanded coordinate system, which is why we need to flip the z-axis.
		context.listener.OrientFront = { forward.x, forward.y, -forward.z };
		context.listener.OrientTop = { up.x, up.y, -up.z };
		context.listener.Position = { position.x, position.y, -position.z };
		context.listener.Velocity = { velocity.x, velocity.y, -velocity.z };
	}

	void update_audio(float dt)
	{
		CPU_PROFILE_BLOCK("Update audio");

		master_audio_settings.volume = max(0.f, master_audio_settings.volume);

		if (old_master_audio_settings.volume != master_audio_settings.volume)
		{
			master_volume_fader.start_fade(master_audio_settings.volume, 0.1f);
		}

		if (old_master_audio_settings.reverb_enabled != master_audio_settings.reverb_enabled || old_master_audio_settings.reverb_preset != master_audio_settings.reverb_preset)
		{
			set_reverb();
		}

		old_master_audio_settings = master_audio_settings;

		master_volume_fader.update(dt);
		context.master_voice->SetVolume(master_volume_fader.current);

		for (uint32 i = 0; i < SOUND_TYPE_COUNT; ++i)
		{
			sound_type_volumes[i] = max(0.f, sound_type_volumes[i]);

			if (old_sound_type_volumes[i] != sound_type_volumes[i])
			{
				sound_type_volume_faders[i].start_fade(sound_type_volumes[i], 0.1f);
			}

			old_sound_type_volumes[i] = sound_type_volumes[i];

			sound_type_volume_faders[i].update(dt);
			context.sound_type_submix_voices[i]->SetVolume(sound_type_volume_faders[i].current);
		}

		ChannelMap::iterator stopped_channels[64];
		uint32 num_stopped_channels = 0;

		for (auto it = channels.begin(), end = channels.end(); it != end; ++it)
		{
			it->second->update(context, dt);
			if (it->second->has_stopped())
			{
				stopped_channels[num_stopped_channels++] = it;
				//LOG_MESSAGE("Deleting channel");
			}
		}

		for (uint32 i = 0; i < num_stopped_channels; ++i)
		{
			channels.erase(stopped_channels[i]);
		}
	}

	SoundHandle play_2d_sound(const SoundID& id, const SoundSettings& settings)
	{
		if (settings.volume <= 0.f)
		{
			return {};
		}

		ref<AudioSound> sound = get_sound(id);
		if (!sound)
		{
			// Not all sounds are file sounds, but we have no chance of creating a synth here.
			if (!load_file_sound(id))
			{
				return {};
			}

			sound = get_sound(id);
		}

		uint32 channel_id = next_channel_id++;

		ref<AudioChannel> channel = make_ref<AudioChannel>(context, sound, settings);
		channels.insert({ channel_id, channel });

		return { channel_id };
	}

	SoundHandle play_3d_sound(const SoundID& id, vec3 position, const SoundSettings& settings)
	{
		if (settings.volume <= 0.f)
		{
			return {};
		}

		ref<AudioSound> sound = get_sound(id);
		if (!sound)
		{
			if (!load_file_sound(id))
			{
				return {};
			}

			sound = get_sound(id);
		}

		uint32 channel_id = next_channel_id++;

		ref<AudioChannel> channel = make_ref<AudioChannel>(context, sound, position, settings);
		channels.insert({ channel_id, channel });

		return { channel_id };
	}

	bool sound_still_playing(SoundHandle handle)
	{
		return handle && (channels.find(handle.id) != channels.end());
	}

	bool stop(SoundHandle handle, float fade_out_time)
	{
		if (handle)
		{
			auto it = channels.find(handle.id);
			if (it != channels.end())
			{
				it->second->stop(fade_out_time);
				return true;
			}
		}
		return false;
	}

	void restart_all_sounds()
	{
		auto channels_copy = std::move(channels);

		for (auto it = channels_copy.begin(), end = channels_copy.end(); it != end; ++it)
		{
			SoundID id = it->second->sound->id;
			SoundSettings settings = *it->second->get_settings();
			bool positioned = it->second->positioned;
			vec3 position = it->second->position;

			it->second->stop(0.f);

			if (positioned)
			{
				play_3d_sound(id, position, settings);
			}
			else
			{
				play_2d_sound(id, settings);
			}
		}
	}

	SoundSettings* get_settings(SoundHandle handle)
	{
		if (handle)
		{
			auto it = channels.find(handle.id);
			if (it != channels.end())
			{
				return it->second->get_settings();
			}
		}
		return 0;
	}

	float db_to_volume(float db)
	{
		return XAudio2DecibelsToAmplitudeRatio(db);
	}

	float volume_to_db(float volume)
	{
		return XAudio2AmplitudeRatioToDecibels(volume);
	}
}