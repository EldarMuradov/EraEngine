// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"
#include "application.h"
#include "geometry/mesh_builder.h"
#include "dx/dx_texture.h"
#include "core/random.h"
#include "core/color.h"
#include "core/imgui.h"
#include "core/log.h"
#include "dx/dx_context.h"
#include "dx/dx_profiling.h"
#include "physics/physics.h"
#include "physics/ragdoll.h"
#include "physics/vehicle.h"
#include "core/threading.h"
#include "rendering/outline.h"
#include "rendering/mesh_shader.h"
#include "rendering/shadow_map.h"
#include "rendering/debug_visualization.h"
#include "scene/scene_rendering.h"
#include "audio/audio.h"
#include "terrain/terrain.h"
#include "terrain/heightmap_collider.h"
#include "terrain/proc_placement.h"
#include "terrain/grass.h"
#include "terrain/water.h"
#include "terrain/tree.h"
#include "animation/skinning.h"
#include "asset/model_asset.h"
#include <px/physics/px_collider_component.h>
#include <asset/file_registry.h>
#include <px/physics/px_joint.h>
#include <px/physics/px_character_controller_component.h>
#include <px/core/px_tasks.h>
#include <px/core/px_aggregate.h>
#include <px/features/px_ragdoll.h>
#include <scripting/script.h>
#include "core/coroutine.h"
#include <ai/navigation.h>
#include <ai/navigation_component.h>
#include <px/features/px_particles.h>
#include "px/features/cloth/px_clothing_factory.h"
#include <ejson_serializer.h>
#include <px/physics/px_soft_body.h>
#include <px/blast/px_blast_destructions.h>

static raytracing_object_type defineBlasFromMesh(const ref<multi_mesh>& mesh)
{
	if (dxContext.featureSupport.raytracing())
	{
		raytracing_blas_builder blasBuilder;
		std::vector<ref<pbr_material>> raytracingMaterials;

		for (auto& sm : mesh->submeshes)
		{
			blasBuilder.push(mesh->mesh.vertexBuffer, mesh->mesh.indexBuffer, sm.info);
			raytracingMaterials.push_back(sm.material);
		}

		ref<raytracing_blas> blas = blasBuilder.finish();
		raytracing_object_type type = pbr_raytracer::defineObjectType(blas, raytracingMaterials);
		return type;
	}
	else
		return {};
}

void addRaytracingComponentAsync(eentity entity, ref<multi_mesh> mesh)
{
	struct add_ray_tracing_data
	{
		eentity entity;
		ref<multi_mesh> mesh;
	};

	add_ray_tracing_data data = { entity, mesh };

	lowPriorityJobQueue.createJob<add_ray_tracing_data>([](add_ray_tracing_data& data, job_handle)
		{
			struct create_component_data
			{
				eentity entity;
				raytracing_object_type blas;
			};

			create_component_data createData = { data.entity, defineBlasFromMesh(data.mesh) };

			mainThreadJobQueue.createJob<create_component_data>([](create_component_data& data, job_handle)
				{
					data.entity.addComponent<raytrace_component>(data.blas);
				}, createData).submitNow();

		}, data).submitAfter(mesh->loadJob);
}

struct updatePhysicsAndScriptingData
{
	float deltaTime{};
	enative_scripting_linker& core;
	escene& scene;
	const user_input& input;
};

void updatePhysXPhysicsAndScripting(escene& currentScene, enative_scripting_linker core, float dt, const user_input& in)
{
	updatePhysicsAndScriptingData data = { dt, core, currentScene, in };

	highPriorityJobQueue.createJob<updatePhysicsAndScriptingData>([](updatePhysicsAndScriptingData& data, job_handle)
		{
			{
				CPU_PROFILE_BLOCK("PhysX steps");

				const auto& physicsRef = physics::physics_holder::physicsRef;

				//lock lock{ physicsRef->sync };

				physicsRef->update(data.deltaTime);

				{
					CPU_PROFILE_BLOCK("PhysX collision events step");

					while (physicsRef->collisionQueue.size())
					{
						const auto& c = physicsRef->collisionQueue.back();
						physicsRef->collisionQueue.pop();
						data.core.handle_coll(c.id1, c.id2);
					}

					while (physicsRef->collisionExitQueue.size())
					{
						const auto& c = physicsRef->collisionExitQueue.back();
						physicsRef->collisionExitQueue.pop();
						data.core.handle_exit_coll(c.id1, c.id2);
					}
				}
			}

			updateScripting(data);

			{
				CPU_PROFILE_BLOCK(".NET 8 Input sync step");
				data.core.handleInput(reinterpret_cast<uintptr_t>(&data.input.keyboard[0]));
			}
		}, data).submitNow();

		const auto& nav_objects = data.scene.group(component_group<navigation_component, transform_component>);

		if (nav_objects.size())
		{
			struct nav_process_data
			{
				decltype(nav_objects) objects;
			};

			nav_process_data nav_data{ nav_objects };

			lowPriorityJobQueue.createJob<nav_process_data>([](nav_process_data& data, job_handle)
				{
					CPU_PROFILE_BLOCK("Navigation step");
					for (auto [entityHandle, nav, transform] : data.objects.each())
					{
						nav.processPath();
					}
				}, nav_data).submitNow();
		}
}

void updateScripting(updatePhysicsAndScriptingData& data)
{
	CPU_PROFILE_BLOCK(".NET 8.0 scripting step");
	for (auto [entityHandle, transform, script] : data.scene.group(component_group<transform_component, script_component>).each())
	{
		const auto& mat = trsToMat4(transform);
		constexpr size_t mat_size = 16;
		float* ptr = new float[mat_size];
		for (size_t i = 0; i < mat_size; i++)
			ptr[i] = mat.m[i];
		data.core.process_trs(reinterpret_cast<uintptr_t>(ptr), (int)entityHandle);
	}
	data.core.update(data.deltaTime);
}

static void initializeAnimationComponentAsync(eentity entity, ref<multi_mesh> mesh)
{
	struct add_animation_data
	{
		eentity entity;
		ref<multi_mesh> mesh;
	};

	add_animation_data data = { entity, mesh };

	mainThreadJobQueue.createJob<add_animation_data>([](add_animation_data& data, job_handle job)
		{
			data.entity.getComponent<animation_component>().animation.set(&data.mesh->skeleton.clips[0]);
		}, data).submitAfter(mesh->loadJob);
}

void application::loadCustomShaders()
{
	if (dxContext.featureSupport.meshShaders())
	{
		initializeMeshShader();
	}
}

static entity_handle cloth{};
static entity_handle particles{};
static entity_handle px_sphere{};
static entity_handle manager{};

void application::initialize(main_renderer* renderer, editor_panels* editorPanels)
{
	this->renderer = renderer;

	if (dxContext.featureSupport.raytracing())
	{
		raytracingTLAS.initialize();
	}

	scene.camera.initializeIngame(vec3(0.f, 1.f, 5.f), quat::identity, deg2rad(70.f), 0.2f);
	scene.editor_camera.initializeIngame(vec3(0.f, 1.f, 5.f), quat::identity, deg2rad(70.f), 0.2f);
	scene.environment.setFromTexture("assets/sky/sunset_in_the_chalk_quarry_4k.hdr");
	scene.environment.lightProbeGrid.initialize(vec3(-20.f, -1.f, -20.f), vec3(40.f, 20.f, 40.f), 1.5f);

	physics::physics_holder::physicsRef = make_ref<physics::px_physics_engine>(*this);

	escene& scene = this->scene.getCurrentScene();

	{
		CPU_PROFILE_BLOCK("Binding for scripting initialization");
		linker.app = this;
		linker.init();
	}

#ifndef ERA_RUNTIME

	editor.initialize(&this->scene, renderer, editorPanels);
	editor.app = this;

#else

	rt.initialize(&this->scene, renderer);

#endif

#ifndef ERA_RUNTIME
	if (auto mesh = loadMeshFromFileAsync("assets/Sponza/sponza.obj"))
	{
		model_asset ass = load3DModelFromFile("assets/Sponza/sponza.obj");
		const auto& sponza = scene.createEntity("Sponza")
			.addComponent<transform_component>(vec3(0.f, 0.f, 0.f), quat::identity, 0.01f)
			//.addComponent<physics::px_convex_mesh_collider_component>(&(ass.meshes[0]))
			//.addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::Static)
			.addComponent<mesh_component>(mesh);

		addRaytracingComponentAsync(sponza, mesh);
	}
#endif

#if 0
	if (auto treeMesh = loadTreeMeshFromFileAsync("assets/tree/source/tree.fbx"))
	{
		auto tree = scene.createEntity("Tree")
			.addComponent<transform_component>(vec3(0.0, 0.0f, 0.0f), quat::identity, 0.1f)
			.addComponent<mesh_component>(treeMesh)
			.addComponent<tree_component>();
	}
#endif

#ifndef ERA_RUNTIME
	{
		auto defaultmat = createPBRMaterialAsync({ "", "" });
		defaultmat->shader = pbr_material_shader_double_sided;

		mesh_builder builder;

		auto sphereMesh = make_ref<multi_mesh>();
		builder.pushSphere({ });
		sphereMesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });

		auto boxMesh = make_ref<multi_mesh>();
		builder.pushBox({ vec3(0.f), vec3(1.f, 1.f, 2.f) });
		boxMesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });

		pbr_material_desc defaultPlaneMatDesc;
		defaultPlaneMatDesc.albedo = "assets/uv.jpg";
		defaultPlaneMatDesc.normal = "";
		defaultPlaneMatDesc.roughness = "";
		defaultPlaneMatDesc.uvScale = 15.0f;
		defaultPlaneMatDesc.metallicOverride = 0.35f;
		defaultPlaneMatDesc.roughnessOverride = 0.01f;

		auto defaultPlaneMat = createPBRMaterialAsync(defaultPlaneMatDesc);

		auto groundMesh = make_ref<multi_mesh>();
		builder.pushBox({ vec3(0.f), vec3(30.f, 4.f, 30.f) });
		groundMesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultPlaneMat });

		//model_asset ass = load3DModelFromFile("assets/sphere.fbx");
		auto& px_sphere_entt = scene.createEntity("SpherePX", (entity_handle)60)
			.addComponent<transform_component>(vec3(-10.0f, 5.0f, -3.0f), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.f))
			.addComponent<mesh_component>(sphereMesh)
			//.addComponent<physics::px_convex_mesh_collider_component>(&(ass.meshes[0]))
			//.addComponent<physics::px_triangle_mesh_collider_component>(&(ass.meshes[0]))
			//.addComponent<physics::px_bounding_box_collider_component>(&(ass.meshes[0]))
			.addComponent<physics::px_sphere_collider_component>(1.0f)
			.addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::Dynamic);
		px_sphere_entt.getComponent<physics::px_rigidbody_component>().setMass(1000.f);
		px_sphere = px_sphere_entt.handle;

		auto px_sphere1 = &scene.createEntity("SpherePX1", (entity_handle)59)
			.addComponent<transform_component>(vec3(5, 155.f, 5), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(5.f))
			.addComponent<mesh_component>(sphereMesh)
			.addComponent<physics::px_sphere_collider_component>(5.0f)
			.addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::Dynamic);
		px_sphere1->getComponent<physics::px_rigidbody_component>().setMass(500.0f);

		{
			if (auto mesh = loadMeshFromFileAsync("assets/obj/bunny.obj"))
			{
				model_asset ass = load3DModelFromFile("assets/obj/bunny.obj");

				auto& px_sphere_entt1 = scene.createEntity("BlastPXTest")
					.addComponent<transform_component>(vec3(0.0f, 0.0f, 0.0f), quat::identity, vec3(1.0f))
					.addComponent<mesh_component>(mesh);

				physics::fracture fracture;
				auto ref = make_ref<submesh_asset>(ass.meshes[0].submeshes[0]);
				unsigned int seed = 7249U;
				manager = fracture.fractureGameObject(ref, px_sphere_entt1, physics::anchor::None, seed, 1, defaultmat, defaultmat, 1.0f, 3.0f);
				scene.deleteEntity(px_sphere_entt1.handle);
			}
		}

		/*{
			model_asset ass = load3DModelFromFile("assets/box.fbx");
			auto px_sphere_entt1 = scene.createEntity("SpherePXTest", (entity_handle)60)
				.addComponent<transform_component>(vec3(0, 0.f, 0.f), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.0f))
				.addComponent<mesh_component>(boxMesh);

			physics::fracture fracture;
			auto ref = make_ref<submesh_asset>(ass.meshes[0].submeshes[0]);

			manager = fracture.fractureGameObject(ref, px_sphere_entt1, physics::anchor::Bottom, 10, 50, defaultmat, defaultmat, 1.0f, 5000);
			scene.deleteEntity(px_sphere_entt1.handle);
		}*/

		//auto soft_body = &scene.createEntity("SoftBody")
		//	.addComponent<transform_component>(vec3(0.f), quat::identity, vec3(1.f))
		//	.addComponent<physics::px_soft_body_component>();

		/*auto px_sphere2 = &scene.createEntity("SpherePX1")
			.addComponent<transform_component>(vec3(8, 5.f, 8), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.f))
			.addComponent<mesh_component>(sphereMesh)
			.addComponent<physics::px_sphere_collider_component>(1.0f)
			.addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::Dynamic);

		auto px_sphere3 = &scene.createEntity("SpherePX1")
			.addComponent<transform_component>(vec3(8, 5.f, 5), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.f))
			.addComponent<mesh_component>(sphereMesh)
			.addComponent<physics::px_sphere_collider_component>(1.0f)
			.addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::Dynamic);

		auto px_sphere4 = &scene.createEntity("SpherePX1")
			.addComponent<transform_component>(vec3(10, 5.f, 12), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.f))
			.addComponent<mesh_component>(sphereMesh)
			.addComponent<physics::px_sphere_collider_component>(1.0f)
			.addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::Dynamic);*/

			//px_sphere1->addChild(*px_sphere);

			/*{
				for (int i = 0; i < 10; i++)
				{
					for (int j = 0; j < 10; j++)
					{
						for (int k = 0; k < 10; k++)
						{
							auto sphr = &scene.createEntity((std::to_string(i) + std::to_string(j) + std::to_string(k)).c_str())
								.addComponent<transform_component>(vec3(2.0f * i, 5 + 2.0f * j + 5, 2.0f * k), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.f))
								.addComponent<mesh_component>(sphereMesh)
								.addComponent<physics::px_sphere_collider_component>(1.0f)
								.addComponent<physics::px_rigidbody_component>(physics::px_rigidbody_type::Dynamic);
						}
					}
				}
			}*/

			//auto px_cct = &scene.createEntity("CharacterControllerPx")
			//	.addComponent<transform_component>(vec3(20.f, 5, -5.f), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.f))
			//	.addComponent<physics::px_box_cct_component>(1.0f, 0.5f, 1.0f);

		auto px_plane = &scene.createEntity("PlanePX")
			.addComponent<transform_component>(vec3(0.f, -2.0, 0.0f), quat::identity, vec3(1.f))
			.addComponent<physics::px_plane_collider_component>();

		/*particles = scene.createEntity("ParticlesPX")
			.addComponent<transform_component>(vec3(0.f, 10.0f, 0.0f), quat::identity, vec3(1.f))
			.addComponent<physics::px_particles_component>(10, 10, 10, false).handle;*/

		//cloth = scene.createEntity("ClothPX")
		//	.addComponent<transform_component>(vec3(0.f, 15.0f, 0.0f), eulerToQuat(vec3(0.0f, 0.0f, 0.0f)), vec3(1.f))
		//	.addComponent<physics::px_cloth_component>(100, 100, vec3(0.f, 15.0f, 0.0f)).handle;

		scene.createEntity("Platform")
			.addComponent<transform_component>(vec3(10, -6.f, 0.f), quat(vec3(1.f, 0.f, 0.f), deg2rad(0.f)))
			.addComponent<mesh_component>(groundMesh);

		auto chainMesh = make_ref<multi_mesh>();

		groundMesh->mesh =
			boxMesh->mesh =
			sphereMesh->mesh =
			chainMesh->mesh =
			builder.createDXMesh();
	}
#endif

	this->scene.sun.direction = normalize(vec3(-0.6f, -1.f, -0.3f));
	this->scene.sun.color = vec3(1.f, 0.93f, 0.76f);
	this->scene.sun.intensity = 11.1f;

	this->scene.sun.numShadowCascades = 3;
	this->scene.sun.shadowDimensions = 2048;
	this->scene.sun.cascadeDistances = vec4(9.f, 25.f, 50.f, 10000.f);
	this->scene.sun.bias = vec4(0.000588f, 0.000784f, 0.000824f, 0.0035f);
	this->scene.sun.blendDistances = vec4(5.f, 10.f, 10.f, 10.f);
	this->scene.sun.stabilize = true;

	for (uint32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
	{
		pointLightBuffer[i] = createUploadBuffer(sizeof(point_light_cb), 512, 0);
		spotLightBuffer[i] = createUploadBuffer(sizeof(spot_light_cb), 512, 0);
		decalBuffer[i] = createUploadBuffer(sizeof(pbr_decal_cb), 512, 0);

		spotLightShadowInfoBuffer[i] = createUploadBuffer(sizeof(spot_shadow_info), 512, 0);
		pointLightShadowInfoBuffer[i] = createUploadBuffer(sizeof(point_shadow_info), 512, 0);

		SET_NAME(pointLightBuffer[i]->resource, "Point lights");
		SET_NAME(spotLightBuffer[i]->resource, "Spot lights");
		SET_NAME(decalBuffer[i]->resource, "Decals");

		SET_NAME(spotLightShadowInfoBuffer[i]->resource, "Spot light shadow infos");
		SET_NAME(pointLightShadowInfoBuffer[i]->resource, "Point light shadow infos");
	}

#if 0
	fireParticleSystem.initialize(10000, 50.f, "assets/particles/fire_explosion.png", 6, 6, vec3(0, 1, 30));
	smokeParticleSystem.initialize(10000, 500.f, "assets/particles/smoke1.tif", 5, 5, vec3(0, 1, 15));
	//boidParticleSystem.initialize(10000, 2000.f);
	debrisParticleSystem.initialize(10000);
#endif

	stackArena.initialize();

	{
		core = make_ref<escripting_core>();
		core->init();
	}

	physics::physics_holder::physicsRef->start();
}

#if 0
bool editFireParticleSystem(fire_particle_system& particleSystem)
{
	bool result = false;
	if (ImGui::BeginTree("Fire particles"))
	{
		if (ImGui::BeginProperties())
		{
			result |= ImGui::PropertySlider("Emit rate", particleSystem.emitRate, 0.f, 1000.f);
			ImGui::EndProperties();
		}

		result |= ImGui::Spline("Size over lifetime", ImVec2(200, 200), particleSystem.settings.sizeOverLifetime);
		ImGui::Separator();
		result |= ImGui::Spline("Atlas progression over lifetime", ImVec2(200, 200), particleSystem.settings.atlasProgressionOverLifetime);
		ImGui::Separator();
		result |= ImGui::Spline("Intensity over lifetime", ImVec2(200, 200), particleSystem.settings.intensityOverLifetime);

		ImGui::EndTree();
	}
	return result;
}

bool editBoidParticleSystem(boid_particle_system& particleSystem)
{
	bool result = false;
	if (ImGui::BeginTree("Boid particles"))
	{
		if (ImGui::BeginProperties())
		{
			result |= ImGui::PropertySlider("Emit rate", particleSystem.emitRate, 0.f, 5000.f);
			result |= ImGui::PropertySlider("Emit radius", particleSystem.settings.radius, 5.f, 100.f);
			ImGui::EndProperties();
		}

		ImGui::EndTree();
	}
	return result;
}

#endif

void application::resetRenderPasses()
{
	opaqueRenderPass.reset();
	transparentRenderPass.reset();
	ldrRenderPass.reset();
	sunShadowRenderPass.reset();
	computePass.reset();

	for (uint32 i = 0; i < arraysize(spotShadowRenderPasses); ++i)
	{
		spotShadowRenderPasses[i].reset();
	}

	for (uint32 i = 0; i < arraysize(pointShadowRenderPasses); ++i)
	{
		pointShadowRenderPasses[i].reset();
	}
}

void application::submitRendererParams(uint32 numSpotLightShadowPasses, uint32 numPointLightShadowPasses)
{
	{
		CPU_PROFILE_BLOCK("Sort render passes");

		opaqueRenderPass.sort();
		transparentRenderPass.sort();
		ldrRenderPass.sort();

		for (uint32 i = 0; i < sunShadowRenderPass.numCascades; ++i)
		{
			sunShadowRenderPass.cascades[i].sort();
		}

		for (uint32 i = 0; i < numSpotLightShadowPasses; ++i)
		{
			spotShadowRenderPasses[i].sort();
		}

		for (uint32 i = 0; i < numPointLightShadowPasses; ++i)
		{
			pointShadowRenderPasses[i].sort();
		}
	}

	renderer->submitRenderPass(&opaqueRenderPass);
	renderer->submitRenderPass(&transparentRenderPass);
	renderer->submitRenderPass(&ldrRenderPass);
	renderer->submitComputePass(&computePass);

	renderer->submitShadowRenderPass(&sunShadowRenderPass);

	for (uint32 i = 0; i < numSpotLightShadowPasses; ++i)
	{
		renderer->submitShadowRenderPass(&spotShadowRenderPasses[i]);
	}

	for (uint32 i = 0; i < numPointLightShadowPasses; ++i)
	{
		renderer->submitShadowRenderPass(&pointShadowRenderPasses[i]);
	}
}

void application::update(const user_input& input, float dt)
{
	resetRenderPasses();

	stackArena.reset();

#ifndef ERA_RUNTIME

	bool objectDragged = editor.update(input, &ldrRenderPass, dt);
	editor.render(&ldrRenderPass, dt);

	render_camera& camera = this->scene.isPausable() ? scene.editor_camera : scene.camera;

#else

	bool objectDragged = false;

	render_camera& camera = scene.camera;

	rt.update();

#endif

	directional_light& sun = scene.sun;
	pbr_environment& environment = scene.environment;

	environment.update(sun.direction);
	sun.updateMatrices(camera);
	setAudioListener(camera.position, camera.rotation, vec3(0.f));
	environment.lightProbeGrid.visualize(&opaqueRenderPass);

	escene& scene = this->scene.getCurrentScene();
	float unscaledDt = dt;
	dt *= this->scene.getTimestepScale();

	//for (auto [entityHandle, terrain, position] : scene.group(component_group<terrain_component, position_component>).each())
	//{
	//	eentity entity = { entityHandle, scene };
	//	heightmap_collider_component* collider = entity.getComponentIfExists<heightmap_collider_component>();

	//	terrain.update(position.position, collider);
	//}

	//static float physicsTimer = 0.f;
	//physicsStep(scene, stackArena, physicsTimer, physics_settings(), dt);

	if (this->scene.isPausable())
	{
		updatePhysXPhysicsAndScripting(scene, linker, dt, input);
	}

	{
		CPU_PROFILE_BLOCK("PhysX GPU clothes render step");
		for (auto [entityHandle, cloth, render] : scene.group(component_group<physics::px_cloth_component, physics::px_cloth_render_component>).each())
		{
			cloth.update(false, &ldrRenderPass);
		}
	}

	{
		CPU_PROFILE_BLOCK("PhysX GPU particles render step");
		for (auto [entityHandle, particles, render] : scene.group(component_group<physics::px_particles_component, physics::px_particles_render_component>).each())
		{
			particles.update(true, &ldrRenderPass);
		}
	}

	// Particles
#if 0
	computePass.dt = dt;
	//computePass.updateParticleSystem(&boidParticleSystem);
	computePass.updateParticleSystem(&fireParticleSystem);
	computePass.updateParticleSystem(&smokeParticleSystem);
	computePass.updateParticleSystem(&debrisParticleSystem);

	//boidParticleSystem.render(&transparentRenderPass);
	fireParticleSystem.render(&transparentRenderPass);
	smokeParticleSystem.render(&transparentRenderPass);
	debrisParticleSystem.render(&transparentRenderPass);
#endif

#ifndef ERA_RUNTIME

	eentity selectedEntity = editor.selectedEntity;

#else

	eentity selectedEntity{};

#endif

	//if (renderer->mode != renderer_mode_pathtraced)
	{
		for (auto [entityHandle, anim, mesh, transform] : scene.group(component_group<animation_component, mesh_component, transform_component>).each())
		{
			anim.update(mesh.mesh, stackArena, dt, &transform);

			if (anim.drawSceleton)
				anim.drawCurrentSkeleton(mesh.mesh, transform, &ldrRenderPass);
		}

		scene_lighting lighting;
		lighting.spotLightBuffer = spotLightBuffer[dxContext.bufferedFrameID];
		lighting.pointLightBuffer = pointLightBuffer[dxContext.bufferedFrameID];
		lighting.spotLightShadowInfoBuffer = spotLightShadowInfoBuffer[dxContext.bufferedFrameID];
		lighting.pointLightShadowInfoBuffer = pointLightShadowInfoBuffer[dxContext.bufferedFrameID];
		lighting.spotShadowRenderPasses = spotShadowRenderPasses;
		lighting.pointShadowRenderPasses = pointShadowRenderPasses;
		lighting.maxNumSpotShadowRenderPasses = arraysize(spotShadowRenderPasses);
		lighting.maxNumPointShadowRenderPasses = arraysize(pointShadowRenderPasses);

		renderScene(camera, scene, stackArena, selectedEntity.handle, sun, lighting, !renderer->settings.cacheShadowMap,
			&opaqueRenderPass, &transparentRenderPass, &ldrRenderPass, &sunShadowRenderPass, &computePass, unscaledDt);

		renderer->setSpotLights(spotLightBuffer[dxContext.bufferedFrameID], scene.numberOfComponentsOfType<spot_light_component>(), spotLightShadowInfoBuffer[dxContext.bufferedFrameID]);
		renderer->setPointLights(pointLightBuffer[dxContext.bufferedFrameID], scene.numberOfComponentsOfType<point_light_component>(), pointLightShadowInfoBuffer[dxContext.bufferedFrameID]);

		if (decals.size())
		{
			updateUploadBufferData(decalBuffer[dxContext.bufferedFrameID], decals.data(), (uint32)(sizeof(pbr_decal_cb) * decals.size()));
			renderer->setDecals(decalBuffer[dxContext.bufferedFrameID], (uint32)decals.size(), decalTexture);
		}

#ifndef ERA_RUNTIME

		if (selectedEntity)
		{
			if (point_light_component* pl = selectedEntity.getComponentIfExists<point_light_component>())
			{
				position_component& pc = selectedEntity.getComponent<position_component>();

				renderWireSphere(pc.position, pl->radius, vec4(pl->color, 1.f), &ldrRenderPass);
			}
			else if (spot_light_component* sl = selectedEntity.getComponentIfExists<spot_light_component>())
			{
				position_rotation_component& prc = selectedEntity.getComponent<position_rotation_component>();

				renderWireCone(prc.position, prc.rotation * vec3(0.f, 0.f, -1.f),
					sl->distance, sl->outerAngle * 2.f, vec4(sl->color, 1.f), &ldrRenderPass);
			}
			else if (physics::px_capsule_cct_component* cct = selectedEntity.getComponentIfExists<physics::px_capsule_cct_component>())
			{
				dynamic_transform_component& dtc = selectedEntity.getComponent<dynamic_transform_component>();
				renderWireCapsule(dtc.position, dtc.position + vec3(0, cct->height, 0), cct->radius, vec4(0.107f, 1.0f, 0.0f, 1.0f), &ldrRenderPass);
			}
			else if (physics::px_box_cct_component* cct = selectedEntity.getComponentIfExists<physics::px_box_cct_component>())
			{
				dynamic_transform_component& dtc = selectedEntity.getComponent<dynamic_transform_component>();
				renderWireBox(dtc.position, vec3(cct->halfSideExtent, cct->halfHeight * 2, cct->halfSideExtent), dtc.rotation, vec4(0.107f, 1.0f, 0.0f, 1.0f), &ldrRenderPass);
			}
			else if (physics::px_convex_mesh_collider_component* cm = selectedEntity.getComponentIfExists<physics::px_convex_mesh_collider_component>())
			{
				auto& rb = selectedEntity.getComponent<physics::px_rigidbody_component>();

				physics::physics_holder::physicsRef->lockRead();

				physx::PxShape* shape[1];
				rb.getRigidActor()->getShapes(shape, 1);
				auto geom = (physx::PxConvexMeshGeometry*)cm->getGeometry();
				auto mesh = geom->convexMesh;
				
				auto vertices = mesh->getVertices();
				auto nbv = mesh->getNbVertices();
				
				for (size_t i = 0; i < nbv; i++)
				{
					vec3 a = physx::createVec3(vertices[i] + shape[0]->getLocalPose().p) + selectedEntity.getComponent<transform_component>().position;
					renderPoint(a, vec4(1, 0, 0, 1), &ldrRenderPass, true);
				}

				physics::physics_holder::physicsRef->unlockRead();
			}
		}

#endif

		// Tests
		{
			//eentity entityCloth{ cloth, &scene.registry };
			//entityCloth.getComponent<physics::px_cloth_component>().clothSystem->update(true, &ldrRenderPass);

			/*eentity entityParticles{ particles, &scene.registry };
			entityParticles.getComponent<physics::px_particles_component>().particleSystem->update(true, &ldrRenderPass);*/
			eentity sphere{ px_sphere, &scene.registry };
			if (input.keyboard['G'].pressEvent)
			{
				//if (!shoted)
				//{
					sphere.getComponent<physics::px_rigidbody_component>().addForce(vec3(500.f, 1.0f, 0.0f), physics::px_force_mode::Impulse);
			    //}
				//physics::physics_holder::physicsRef->explode(vec3(0.0f, 5.0f, 3.0f), 15.0f, 300.0f);
				//entityCloth.getComponent<physics::px_cloth_component>().clothSystem->translate(vec3(0.f, 2.f, 0.f));
				//entityParticles.getComponent<px_particles_component>().particleSystem->translate(PxVec4(0.f, 20.f, 0.f, 0.f));
			}

			//{
				/*eentity entt{ manager, &scene.registry };
				auto& chunkManager = entt.getComponent<physics::chunk_graph_manager>();
				chunkManager.update();

				for (size_t i = 0; i < chunkManager.nbNodes; i++)
				{
					eentity node{ chunkManager.nodes[i], &scene.registry };

					node.getComponent<physics::chunk_graph_manager::chunk_node>().update();
				}*/
			//}

			// Render soft bodies
			if (!physics::physics_holder::physicsRef->softBodies.empty())
			{
				const auto positions = physics::physics_holder::physicsRef->softBodies[0]->positionsInvMass;
				const auto nbVerts = physics::physics_holder::physicsRef->softBodies[0]->getNbVertices();

				for (size_t i = 0; i < nbVerts; i++)
				{
					vec3 pos = vec3(positions[i].x, positions[i].y, positions[i].z);
					renderPoint(pos, vec4(1.0f, 0.0f, 0.0f, 1.f), &ldrRenderPass);
				}
			}

			processPoints();
		}

		submitRendererParams(lighting.numSpotShadowRenderPasses, lighting.numPointShadowRenderPasses);
	}

	for (auto [entityHandle, transform, dynamic] : scene.group(component_group<transform_component, dynamic_transform_component>).each())
	{
		dynamic = transform;
	}

	performSkinning(&computePass);

	if (dxContext.featureSupport.raytracing())
	{
		raytracingTLAS.reset();

		for (auto [entityHandle, transform, raytrace] : scene.group(component_group<transform_component, raytrace_component>).each())
		{
			auto handle = raytracingTLAS.instantiate(raytrace.type, transform);
		}

		renderer->setRaytracingScene(&raytracingTLAS);
	}

	renderer->setEnvironment(environment);
	renderer->setSun(sun);
	renderer->setCamera(camera);

	executeMainThreadJobs();
}

void application::handleFileDrop(const fs::path& filename)
{
	fs::path path = filename;
	fs::path relative = fs::relative(path, fs::current_path());
	fs::path ext = relative.extension();

	if (isMeshExtension(ext))
	{
		if (auto mesh = loadAnimatedMeshFromFileAsync(relative.string()))
		{
			fs::path path = filename;
			path = path.stem();

			auto& en = scene.getCurrentScene().createEntity(path.string().c_str())
				.addComponent<transform_component>(vec3(0.f), quat::identity)
				.addComponent<animation_component>()
				.addComponent<dynamic_transform_component>()
				.addComponent<mesh_component>(mesh);
			initializeAnimationComponentAsync(en, mesh);
			addRaytracingComponentAsync(en, mesh);
		}
		else if (auto mesh = loadMeshFromFileAsync(relative.string()))
		{
			fs::path path = filename;
			path = path.stem();

			auto& en = scene.getCurrentScene().createEntity(path.string().c_str())
				.addComponent<transform_component>(vec3(0.f), quat::identity)
				.addComponent<mesh_component>(mesh);

			addRaytracingComponentAsync(en, mesh);
		}
	}
	else if (ext == ".hdr")
	{
		scene.environment.setFromTexture(relative);
	}
}

void application::renderObjectPoint(float x, float y, float z)
{
	renderPoint(vec3(x, y, z), vec4(1.0f, 0.0f, 0.0f, 1.f), &ldrRenderPass);
}

void application::renderObjectBox(vec3 pos, float x, float y, float z)
{
	renderWireBox(pos, vec3(x, y, z), quat::identity, vec4(1.0f, 0.0f, 0.0f, 1.f), &ldrRenderPass);
}

void application::renderObjectSphere(vec3 pos, float radius)
{
	renderWireSphere(pos, radius, vec4(1.0f, 0.0f, 0.0f, 1.f), &ldrRenderPass);
}

void application::processPoints()
{
	for (auto p : points)
		renderPoint(p, vec4(1.0f, 0.0f, 0.0f, 1.f), &ldrRenderPass);
}