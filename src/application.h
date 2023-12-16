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

void addRaytracingComponentAsync(eentity entity, ref<multi_mesh> mesh);

bool editFireParticleSystem(fire_particle_system& particleSystem);
bool editBoidParticleSystem(boid_particle_system& particleSystem);

void updatePhysXPhysicsAndScripting(escene& currentScene, std::shared_ptr<escripting_core> core, float dt);
void updateScripting(std::shared_ptr<escripting_core> core, float dt);

struct application
{
	void loadCustomShaders();
	void initialize(main_renderer* renderer, editor_panels* editorPanels);
	void update(const user_input& input, float dt);

	void handleFileDrop(const fs::path& filename);

	escene* getCurrentScene() { return &scene.getCurrentScene(); }

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
	eeditor editor;

	fire_particle_system fireParticleSystem;
	smoke_particle_system smokeParticleSystem;
	boid_particle_system boidParticleSystem;
	debris_particle_system debrisParticleSystem;

	eallocator stackArena;

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