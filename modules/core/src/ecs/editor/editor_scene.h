#pragma once

#include "ecs/world.h"

#include "rendering/light_source.h"
#include "rendering/pbr.h"
#include "rendering/pbr_environment.h"

#include "core/camera.h"

namespace era_engine
{

	class EditorScene final
	{
	public:
		enum SceneMode
		{
			scene_mode_editor,
			scene_mode_runtime_playing,
			scene_mode_runtime_paused,
		};

		EditorScene()
		{
		}

		static void init()
		{
		}

		static ref<World> get_current_world()
		{
			return runtime_world;
		}

		float get_timestep_scale() const
		{
			return (mode == scene_mode_editor || mode == scene_mode_runtime_paused) ? 0.f : timestep_scale;
		}

		void play()
		{
			if (mode == scene_mode_editor)
			{
				//runtime_world->destroy(false);
			}
			mode = scene_mode_runtime_playing;
		}

		void pause()
		{
			if (mode == scene_mode_runtime_playing)
			{
				mode = scene_mode_runtime_paused;
			}
		}

		void stop()
		{
			mode = scene_mode_editor;
		}

		bool is_playable() const
		{
			return mode == scene_mode_editor || mode == scene_mode_runtime_paused;
		}

		bool is_pausable() const
		{
			return mode == scene_mode_runtime_playing;
		}

		bool is_stoppable() const
		{
			return mode == scene_mode_runtime_playing || mode == scene_mode_runtime_paused;
		}

		static inline ref<World> runtime_world = nullptr;

		static inline SceneMode mode = scene_mode_runtime_playing;
		float timestep_scale = 1.0f;

		render_camera camera;
		render_camera editor_camera;

		directional_light sun;
		pbr_environment environment;

		fs::path save_path;
	};
}