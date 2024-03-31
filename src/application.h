// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/input.h"
#include "core/camera.h"
#include "core/camera_controller.h"
#include "geometry/mesh.h"
#include "core/math.h"
#include "scene/scene.h"
#include "rendering/main_renderer.h"
#include "particles/fire_particle_system.h"
#include "particles/smoke_particle_system.h"
#include "particles/boid_particle_system.h"
#include "particles/debris_particle_system.h"
#include "rendering/raytracing.h"
#include "editor/editor.h"
#include "learning/learned_locomotion.h"
#include <px/core/px_physics_engine.h>
#include <EraScriptingLauncher-Lib/scripting_core.h>
#include <scripting/native_scripting_linker.h>

#if ERA_RUNTIME

#include "core/runtime.h"

#endif

void addRaytracingComponentAsync(eentity entity, ref<multi_mesh> mesh);

bool editFireParticleSystem(fire_particle_system& particleSystem);
bool editBoidParticleSystem(boid_particle_system& particleSystem);

struct updatePhysicsAndScriptingData;
void updatePhysXPhysicsAndScripting(escene& currentScene, enative_scripting_linker core, float dt, const user_input& in);
void updateScripting(updatePhysicsAndScriptingData& data);

struct application
{
	void loadCustomShaders();
	void initialize(main_renderer* renderer, editor_panels* editorPanels);
	void update(const user_input& input, float dt);

	void handleFileDrop(const fs::path& filename);

	NODISCARD escene* getCurrentScene() { return &scene.getCurrentScene(); }
	NODISCARD editor_scene* getScene() { return &scene; }

	void renderObjectPoint(float x, float y, float z);

	eallocator stackArena;

	main_renderer* getRenderer() const noexcept
	{
		return renderer;
	}

	std::vector<vec3> points;
	void processPoints();

#ifndef ERA_RUNTIME

	eeditor* getEditor() noexcept
	{
		return &editor;
	}

#endif

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

	enative_scripting_linker linker;

	std::shared_ptr<escripting_core> core;

	editor_scene scene;

#ifndef ERA_RUNTIME

	eeditor editor;

#else

	runtime rt;

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

	friend class px_physics_engine;
	friend class eeditor;
};