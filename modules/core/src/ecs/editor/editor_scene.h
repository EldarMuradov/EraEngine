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
            SCENE_MODE_EDITOR,
            SCENE_MODE_RUNTIME_PLAYING,
            SCENE_MODE_RUNTIME_PAUSED,
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
            return (mode == SCENE_MODE_EDITOR || mode == SCENE_MODE_RUNTIME_PAUSED) ? 0.f : timestep_scale;
        }

        void play()
        {
            if (mode == SCENE_MODE_EDITOR)
            {
                // runtime_world->destroy(false);
            }
            mode = SCENE_MODE_RUNTIME_PLAYING;
        }

        void pause()
        {
            if (mode == SCENE_MODE_RUNTIME_PLAYING)
            {
                mode = SCENE_MODE_RUNTIME_PAUSED;
            }
        }

        void stop()
        {
            mode = SCENE_MODE_EDITOR;
        }

        bool is_playable() const
        {
            return mode == SCENE_MODE_EDITOR || mode == SCENE_MODE_RUNTIME_PAUSED;
        }

        bool is_pausable() const
        {
            return mode == SCENE_MODE_RUNTIME_PLAYING;
        }

        bool is_stoppable() const
        {
            return mode == SCENE_MODE_RUNTIME_PLAYING || mode == SCENE_MODE_RUNTIME_PAUSED;
        }

        static inline ref<World> runtime_world = nullptr;

        static inline SceneMode mode = SCENE_MODE_RUNTIME_PLAYING;
        float timestep_scale = 1.0f;

        RenderCamera camera;
        RenderCamera editor_camera;

        DirectionalLight sun;
        PBREnvironment environment;

        fs::path save_path;
    };
}