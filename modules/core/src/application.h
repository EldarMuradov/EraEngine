// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"
#include "core/input.h"
#include "core/camera.h"
#include "core/camera_controller.h"

#include "geometry/mesh.h"

#include "scene/scene.h"

#include "particles/fire_particle_system.h"
#include "particles/smoke_particle_system.h"
#include "particles/boid_particle_system.h"
#include "particles/debris_particle_system.h"

#include "rendering/raytracing.h"
#include "rendering/main_renderer.h"

#include "learning/learned_locomotion.h"

#include "px/core/px_physics_engine.h"

#ifdef ERA_RUNTIME
#include "core/runtime.h"
#endif

#include "editor/editor.h"

#include "ecs/editor/editor_scene.h"

namespace era_engine
{
	namespace dotnet
	{
		struct enative_scripting_linker;
	}

	void addRaytracingComponentAsync(Entity entity, ref<multi_mesh> mesh);

	bool editFireParticleSystem(fire_particle_system& particleSystem);
	bool editBoidParticleSystem(boid_particle_system& particleSystem);

	struct update_scripting_data;
	void updatePhysXCallbacksAndScripting(ref<World> current_world, ref<dotnet::enative_scripting_linker> core, float dt, const user_input& in);
	void updateScripting(update_scripting_data& data);

	struct application
	{
		application() {}
		void loadCustomShaders();
		void initialize(main_renderer* renderer, editor_panels* editorPanels);
		void update(const user_input& input, float dt);

		void handleFileDrop(const fs::path& filename);

		escene* getCurrentScene() { return &scene.getCurrentScene(); }
		editor_scene* getScene() { return &scene; }

		ref<World> getCurrentWorld() const { return world_scene->get_current_world(); }
		ref<EditorScene> getWorldScene() { return world_scene; }

		main_renderer* getRenderer() const
		{
			return renderer;
		}

#ifndef ERA_RUNTIME

		eeditor* getEditor()
		{
			return &editor;
		}

#endif

		eallocator stackArena;

	private:
		void resetRenderPasses();
		void submitRendererParams(uint32 numSpotLightShadowPasses, uint32 numPointLightShadowPasses);

		raytracing_tlas raytracingTLAS;

		ref<dx_buffer> pointLightBuffer[NUM_BUFFERED_FRAMES];
		ref<dx_buffer> spotLightBuffer[NUM_BUFFERED_FRAMES];
		ref<dx_buffer> decalBuffer[NUM_BUFFERED_FRAMES];

		ref<dx_buffer> spotLightShadowInfoBuffer[NUM_BUFFERED_FRAMES];
		ref<dx_buffer> pointLightShadowInfoBuffer[NUM_BUFFERED_FRAMES];

		std::vector<pbr_decal_cb> decals;

		ref<dx_texture> decalTexture;

		main_renderer* renderer;

		ref<dotnet::enative_scripting_linker> linker;

		editor_scene scene;

		ref<EditorScene> world_scene = nullptr;

#ifndef ERA_RUNTIME

		eeditor editor;

#else

		runtime::runtime rt;

#endif

		fire_particle_system fireParticleSystem;
		smoke_particle_system smokeParticleSystem;
		boid_particle_system boidParticleSystem;
		debris_particle_system debrisParticleSystem;

		learned_locomotion learnedLocomotion;

		opaque_render_pass opaqueRenderPass;
		transparent_render_pass transparentRenderPass;
		sun_shadow_render_pass sunShadowRenderPass;
		spot_shadow_render_pass spotShadowRenderPasses[16];
		point_shadow_render_pass pointShadowRenderPasses[16];
		ldr_render_pass ldrRenderPass;
		compute_pass computePass;

		friend class physics::px_physics_engine;
		friend class eeditor;
	};

	extern application globalApp;
}