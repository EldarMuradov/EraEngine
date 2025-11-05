#pragma once

#include "physics_api.h"

#include "physics/destructions/blast_physx/NvBlastExtSync.h"

#include <chrono>

namespace era_engine::physics
{
	class ERA_PHYSICS_API DestructionReplay
	{
	public:
		DestructionReplay();
		virtual ~DestructionReplay();

		bool has_record() const;

		size_t get_event_count() const;

		uint32 get_current_event_index() const;

		void add_family(Nv::Blast::TkFamily* family);
		void remove_family(Nv::Blast::TkFamily* family);

		void start_recording(Nv::Blast::ExtPxManager& manager, bool sync_family, bool sync_physics);
		void stop_recording();

		void start_playback(Nv::Blast::ExtPxManager& manager, Nv::Blast::TkGroup* tk_group);
		void stop_playback();

		void update();
		void reset();

	private:
		void clear_buffer();

	private:

		Nv::Blast::ExtPxManager* px_manager = nullptr;

		Nv::Blast::TkGroup* group = nullptr;
		Nv::Blast::ExtSync* sync = nullptr;

		std::vector<Nv::Blast::ExtSyncEvent*> buffer;

		std::chrono::steady_clock::time_point start_time;
		uint64 first_event_ts = 0;
		uint32 next_event_index = 0;

		bool is_recording = false;
		bool is_playing = false;
	};
}