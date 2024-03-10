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
#include <EraScriptingLauncher-Lib/src/script.h>
#include "core/coroutine.h"
#include <ai/navigation.h>
#include <ai/navigation_component.h>
#include <px/features/px_particles.h>
#include "px/features/cloth/px_clothing_factory.h"

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
	enative_scripting_linker core;
	escene& scene;
	float deltaTime;
	const user_input& input;
};

void updatePhysXPhysicsAndScripting(escene& currentScene, enative_scripting_linker core, float dt, const user_input& in)
{
	updatePhysicsAndScriptingData data = { core, currentScene, dt, in};

	highPriorityJobQueue.createJob<updatePhysicsAndScriptingData>([](updatePhysicsAndScriptingData& data, job_handle)
	{
		{
			px_physics_engine::update(data.deltaTime);

			while (px_physics_engine::collisionQueue.size())
			{
				const auto& c = px_physics_engine::collisionQueue.back();
				px_physics_engine::collisionQueue.pop();
				data.core.handle_coll(c.id1, c.id2);
			}
		}

		updateScripting(data);

		data.core.handleInput(reinterpret_cast<uintptr_t>(&data.input.keyboard[0]));
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
			for (auto [entityHandle, nav, transform] : data.objects.each())
			{
				nav.processPath();
			}
		}, nav_data).submitNow();
	}
}

void updateScripting(updatePhysicsAndScriptingData& data)
{
	CPU_PROFILE_BLOCK(".NET 8.0 Native AOT scripting step");
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

//px_particle_system* particleSystem = nullptr;
//px_cloth_system* clothSystem = nullptr;
entity_handle cloth{};
entity_handle particles{};

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

#ifndef ERA_RUNTIME

	editor.initialize(&this->scene, renderer, editorPanels);
	editor.app = this;

#else

	rt.initialize(&this->scene, renderer);

#endif

	escene& scene = this->scene.getCurrentScene();

	{
		CPU_PROFILE_BLOCK("Binding for scripting initialization");
		linker.app = this;
		linker.init();
	}

	px_physics_engine::initialize(this);

#ifndef ERA_RUNTIME
	/*if (auto mesh = loadMeshFromFileAsync("assets/Sponza/Sponza.obj"))
	{
		const auto& sponza = scene.createEntity("Sponza")
			.addComponent<transform_component>(vec3(0.f, 1.f, 0.f), quat::identity, 0.01f)
			.addComponent<mesh_component>(mesh);

		addRaytracingComponentAsync(sponza, mesh);
	}*/
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

		mesh_builder builder;

		auto sphereMesh = make_ref<multi_mesh>();
		builder.pushSphere({ });
		sphereMesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });

		auto boxMesh = make_ref<multi_mesh>();
		builder.pushBox({ vec3(0.f), vec3(1.f, 1.f, 2.f) });
		boxMesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });

		pbr_material_desc defaultPlaneMatDesc;
		defaultPlaneMatDesc.albedo = "";
		defaultPlaneMatDesc.normal = "";
		defaultPlaneMatDesc.roughness = "";
		defaultPlaneMatDesc.uvScale = 3.f;

		auto defaultPlaneMat = createPBRMaterialAsync(defaultPlaneMatDesc);

		auto groundMesh = make_ref<multi_mesh>();
		builder.pushBox({ vec3(0.f), vec3(30.f, 4.f, 30.f) });
		groundMesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultPlaneMat });

		//for (uint32 i = 0; i < 3; ++i)
		//{
		//	scene.createEntity("Sphere")
		//		.addComponent<transform_component>(vec3(15.0f, 10.f + i * 3.f, 15.0f), quat(vec3(0.f, 0.f, 1.f), deg2rad(1.f)), vec3(1.f))
		//		.addComponent<mesh_component>(sphereMesh);
		//}

		//model_asset ass = load3DModelFromFile("assets/sphere.fbx");
		auto px_sphere = &scene.createEntity("SpherePX", (entity_handle)60)
			.addComponent<transform_component>(vec3(0, 2.f, 0), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.f))
			.addComponent<mesh_component>(sphereMesh)
			//.addComponent<px_convex_mesh_collider_component>(&(ass.meshes[0]))
			//.addComponent<px_triangle_mesh_collider_component>(&(ass.meshes[0]))
			//.addComponent<px_bounding_box_collider_component>(&(ass.meshes[0]))
			.addComponent<px_sphere_collider_component>(1.0f)
			.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic);

		auto px_sphere1 = &scene.createEntity("SpherePX1")
			.addComponent<transform_component>(vec3(0, 5.f, 0), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.f))
			.addComponent<mesh_component>(sphereMesh)
			.addComponent<px_sphere_collider_component>(1.0f)
			.addComponent<px_rigidbody_component>(px_rigidbody_type::Dynamic);

		px_sphere1->addChild(*px_sphere);

		//auto px_cct = &scene.createEntity("CharacterControllerPx")
		//	.addComponent<transform_component>(vec3(20.f, 5, -5.f), quat(vec3(0.f, 0.f, 0.f), deg2rad(1.f)), vec3(1.f))
		//	.addComponent<px_box_cct_component>();

		auto px_plane = &scene.createEntity("PlanePX")
			.addComponent<transform_component>(vec3(0.f, 0.0f, 0.0f), quat::identity, vec3(1.f))
			.addComponent<px_plane_collider_component>();

		/*auto px_plane_wall1 = &scene.createEntity("PlanePX_Wall1")
			.addComponent<transform_component>(vec3(0.f, 0.0f, 0.0f), quat::identity, vec3(1.f))
			.addComponent<px_plane_collider_component>(vec3(0.f, 0.f, 0.f));*/

		/*particles = scene.createEntity("ParticlesPX")
			.addComponent<transform_component>(vec3(0.f, 10.0f, 0.0f), quat::identity, vec3(1.f))
			.addComponent<px_particles_component>(10, 10, 10).handle;

		cloth = scene.createEntity("ClothPX")
			.addComponent<transform_component>(vec3(0.f, 15.0f, 0.0f), eulerToQuat(vec3(0.0f, 0.0f, 0.0f)), vec3(1.f))
			.addComponent<px_cloth_component>(10, 10, vec3(0.f, 15.0f, 0.0f)).handle;*/

		px_raycast_info rci = px_physics_engine::get()->raycast(&px_sphere1->getComponent<px_rigidbody_component>(), vec3(0, -1, 0));
		if (rci.actor)
		{
			std::cout << "Raycast. Dist: " << rci.distance << '\n';
		}
		else
		{
			std::cout << "Raycast. Dist: " << rci.distance << "\n";
		}

		const auto& overlap_info = px_physics_engine::overlapCapsule(vec3(0, -5, 0), 1.5f, 3.0f, quat::identity, false);

		std::cout << "Overlapping: " << overlap_info.isOverlapping << "\n";
		std::cout << "Results: " << overlap_info.results.size() << "\n";

		editor.physicsSettings.collisionBeginCallback = [rng = random_number_generator{ 519431 }](const collision_begin_event& e) mutable
		{
			float speed = length(e.relativeVelocity);

			sound_settings settings;
			settings.pitch = rng.randomFloatBetween(0.5f, 1.5f);
			settings.volume = saturate(remap(speed, 0.2f, 20.f, 0.f, 1.f));

			play3DSound(SOUND_ID("punch"), e.position, settings);
		};

		editor.physicsSettings.collisionEndCallback = [](const collision_end_event& e) mutable
		{
		};

		scene.createEntity("Platform")
			.addComponent<transform_component>(vec3(10, -4.f, 0.f), quat(vec3(1.f, 0.f, 0.f), deg2rad(0.f)))
			.addComponent<mesh_component>(groundMesh)
			.addComponent<collider_component>(collider_component::asAABB(bounding_box::fromCenterRadius(vec3(0.f, 0.f, 0.f), vec3(30.f, 4.f, 30.f)), { physics_material_type_metal, 0.1f, 1.f, 4.f }));

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
		core = std::make_shared<escripting_core>();
		core->init();
	}
}

#if 0
NODISCARD bool editFireParticleSystem(fire_particle_system& particleSystem)
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

NODISCARD bool editBoidParticleSystem(boid_particle_system& particleSystem)
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

	if (&camera != editor.cameraController.camera)
		editor.cameraController.camera = &camera;

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

	for (auto [entityHandle, terrain, position] : scene.group(component_group<terrain_component, position_component>).each())
	{
		eentity entity = { entityHandle, scene };
		heightmap_collider_component* collider = entity.getComponentIfExists<heightmap_collider_component>();

		terrain.update(position.position, collider);
	}

	static float physicsTimer = 0.f;
	physicsStep(scene, stackArena, physicsTimer, physics_settings(), dt);

	if (this->scene.isPausable())
	{
		updatePhysXPhysicsAndScripting(scene, linker, dt, input);
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

	if (renderer->mode != renderer_mode_pathtraced)
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
			else if (px_capsule_cct_component* cct = selectedEntity.getComponentIfExists<px_capsule_cct_component>())
			{
				dynamic_transform_component& dtc = selectedEntity.getComponent<dynamic_transform_component>();
				renderWireCapsule(dtc.position, dtc.position + vec3(0, cct->getHeight(), 0), cct->getRadius(), vec4(0.107f, 1.0f, 0.0f, 1.0f), &ldrRenderPass);
			}
			else if (px_box_cct_component* cct = selectedEntity.getComponentIfExists<px_box_cct_component>())
			{
				dynamic_transform_component& dtc = selectedEntity.getComponent<dynamic_transform_component>();
				renderWireBox(dtc.position, vec3(cct->getHalfSideExtent(), cct->getHalfHeight() * 2, cct->getHalfSideExtent()), dtc.rotation, vec4(0.107f, 1.0f, 0.0f, 1.0f), &ldrRenderPass);
			}
		}

#endif
	
		//// Tests
		//{
		//	eentity entityCloth{ cloth, &scene.registry };
		//	entityCloth.getComponent<px_cloth_component>().clothSystem->update(true, &ldrRenderPass);

		//	eentity entityParticles{ particles, &scene.registry };
		//	entityParticles.getComponent<px_particles_component>().particleSystem->update(true, &ldrRenderPass);

		//	if (input.keyboard['G'].down)
		//	{
		//		entityCloth.getComponent<px_cloth_component>().clothSystem->translate(PxVec4(0.f, 2.f, 0.f, 0.f));
		//		//entityParticles.getComponent<px_particles_component>().particleSystem->translate(PxVec4(0.f, 20.f, 0.f, 0.f));
		//	}
		//}

		//if (input.keyboard['G'].down)
		//{
		//	//px_physics_engine::get()->getPhysicsAdapter()->pvd->disconnect();
		//}

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
			raytracingTLAS.instantiate(raytrace.type, transform);
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