#include "game/game_init_system.h"

#include <core/ecs/input_reciever_component.h>
#include <core/ecs/input_sender_component.h>
#include <core/string.h>

#include <rendering/mesh_shader.h>
#include <rendering/ecs/renderer_holder_root_component.h>

#include <ecs/update_groups.h>
#include <ecs/base_components/transform_component.h>
#include <ecs/rendering/world_renderer.h>
#include <ecs/rendering/scene_rendering.h>
#include <core/ecs/camera_holder_component.h>
#include <ecs/rendering/mesh_component.h>

#include <game/movement/movement_component.h>

#include <physics/body_component.h>
#include <physics/shape_component.h>
#include <physics/basic_objects.h>

#include <audio/audio.h>

#include <terrain/terrain.h>

#include <animation/skinning.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<GameInitSystem>("GameInitSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("game")))
			.method("update", &GameInitSystem::update)(metadata("update_group", update_types::BEGIN));
	}

	GameInitSystem::GameInitSystem(World* _world)
		: System(_world)
	{
	}

	GameInitSystem::~GameInitSystem()
	{
	}

	void GameInitSystem::init()
	{
		RendererHolderRootComponent* renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);

		Entity camera_entity = world->create_entity("CameraEntity");
		camera_entity.add_component<CameraHolderComponent>()
			.add_component<InputSenderComponent>()
			//.add_component<MovementComponent>()
			.add_component<InputRecieverComponent>();

		CameraHolderComponent& camera_holder_component = camera_entity.get_component<CameraHolderComponent>();
		camera_holder_component.set_camera_type(CameraHolderComponent::FREE_CAMERA);
		camera_holder_component.set_render_camera(&renderer_holder_rc->camera);

		camera_entity.get_component<InputSenderComponent>().add_reciever(camera_entity.get_component_if_exists<InputRecieverComponent>());

		auto defaultmat = createPBRMaterialAsync({ "", "" });
		defaultmat->shader = pbr_material_shader_double_sided;

		mesh_builder builder;

		auto sphereMesh = make_ref<multi_mesh>();
		builder.pushSphere({ });
		sphereMesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });

		auto boxMesh = make_ref<multi_mesh>();
		builder.pushBox({ vec3(0.f), vec3(1.f, 1.f, 2.f) });
		boxMesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultmat });

		PbrMaterialDesc defaultPlaneMatDesc;
		defaultPlaneMatDesc.albedo = get_asset_path("/resources/assets/uv.jpg");
		defaultPlaneMatDesc.normal = "";
		defaultPlaneMatDesc.roughness = "";
		defaultPlaneMatDesc.uv_scale = 15.0f;
		defaultPlaneMatDesc.metallic_override = 0.35f;
		defaultPlaneMatDesc.roughness_override = 0.01f;

		auto defaultPlaneMat = createPBRMaterialAsync(defaultPlaneMatDesc);

		auto groundMesh = make_ref<multi_mesh>();
		builder.pushBox({ vec3(0.f), vec3(30.f, 4.f, 30.f) });
		groundMesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, defaultPlaneMat });

		if (auto mesh = loadAnimatedMeshFromFileAsync(get_asset_path("/resources/assets/resident-evil-2-tyrant/source/UmodelExport.fbx")))
		{
			auto& tiran = world->create_entity("Tiran")
				.add_component<animation::AnimationComponent>()
				.add_component<animation::SkeletonComponent>()
				.add_component<MeshComponent>(mesh);

			TransformComponent& transform_component = tiran.get_component<TransformComponent>();
			transform_component.type = TransformComponent::DYNAMIC;
			transform_component.transform = trs{ vec3(-10.0f, -2.0f, 0.0f), quat(vec3(1.f, 0.f, 0.f), deg2rad(-90.f)), vec3(0.1f) };

			initializeAnimationComponentAsync(tiran, mesh);
			addRaytracingComponentAsync(tiran, mesh);
		}

		if (auto mesh = loadMeshFromFileAsync(get_asset_path("/resources/assets/Sponza/sponza.obj")))
		{
			auto& sponza = world->create_entity("Sponza")
				.add_component<MeshComponent>(mesh);

			TransformComponent& transform_component = sponza.get_component<TransformComponent>();
			transform_component.transform.position = vec3(5.0f, -3.75f, 35.0f);
			transform_component.transform.scale = vec3(0.01f);

			addRaytracingComponentAsync(sponza, mesh);
		}

		auto& physics_sphere = world->create_entity("SpherePX")
			.add_component<MeshComponent>(sphereMesh);
		physics_sphere.get_component<TransformComponent>().transform.position = vec3(-10.0f, 5.0f, -3.0f);
		physics_sphere.add_component<physics::SphereShapeComponent>(1.0f)
					  .add_component<physics::DynamicBodyComponent>();
		physics_sphere.get_component<physics::DynamicBodyComponent>().set_CCD(true);

		auto& big_physics_sphere = world->create_entity("SpherePX1")
			.add_component<MeshComponent>(sphereMesh);
		big_physics_sphere.get_component<TransformComponent>().transform.scale = vec3(5.0f);
		big_physics_sphere.get_component<TransformComponent>().transform.position = vec3(5.0f, 155.f, 5.0f);
		
		big_physics_sphere.add_component<physics::SphereShapeComponent>(5.0f)
						  .add_component<physics::DynamicBodyComponent>();
		big_physics_sphere.get_component<physics::DynamicBodyComponent>().set_CCD(true);

		auto& plane = world->create_entity("Platform")
			.add_component<physics::PlaneComponent>(vec3(0.f, -5.0, 0.0f))
			.add_component<MeshComponent>(groundMesh);
		plane.get_component<TransformComponent>().transform = trs{ vec3(10, -9.f, 0.f), quat(vec3(1.f, 0.f, 0.f), deg2rad(0.f)), vec3(5.0f, 1.0f, 5.0f) };
		
		groundMesh->mesh =
			boxMesh->mesh =
			sphereMesh->mesh =
			builder.createDXMesh();
	}

	void GameInitSystem::update(float dt)
	{
		
	}

}