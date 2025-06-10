#include "game/game_init_system.h"

#include <core/ecs/input_reciever_component.h>
#include <core/ecs/input_sender_component.h>
#include <core/string.h>
#include <core/ecs/camera_holder_component.h>

#include <rendering/mesh_shader.h>
#include <rendering/ecs/renderer_holder_root_component.h>

#include <ecs/update_groups.h>
#include <ecs/base_components/transform_component.h>
#include <ecs/rendering/world_renderer.h>
#include <ecs/rendering/scene_rendering.h>
#include <ecs/rendering/mesh_component.h>

#include <game/movement/movement_component.h>

#include <motion_matching/controller.h>

#include <physics/body_component.h>
#include <physics/shape_component.h>
#include <physics/basic_objects.h>
#include <physics/joint.h>

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
			.method("update", &GameInitSystem::update)(metadata("update_group", update_types::GAMEPLAY_NORMAL));
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
		CameraHolderComponent& camera_holder_component = camera_entity.add_component<CameraHolderComponent>();

		camera_entity.add_component<InputSenderComponent>().add_reciever(&camera_entity.add_component<InputRecieverComponent>());
		camera_entity.add_component<MovementComponent>();
		camera_entity.add_component<MotionMatchingControllerComponent>();

		camera_holder_component.set_camera_type(CameraHolderComponent::FREE_CAMERA);
		camera_holder_component.set_render_camera(&renderer_holder_rc->camera);

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

		Entity tiran;
		if (auto mesh = loadAnimatedMeshFromFileAsync(get_asset_path("/resources/assets/resident-evil-2-tyrant/source/UmodelExport.fbx"), mesh_creation_flags_unreal_animated_asset))
		{
			tiran = world->create_entity("Tiran");
			tiran.add_component<animation::AnimationComponent>();
			tiran.add_component<animation::SkeletonComponent>();
			tiran.add_component<MeshComponent>(mesh);

			TransformComponent& transform_component = tiran.get_component<TransformComponent>();
			transform_component.transform = trs{ vec3(-10.0f, -2.0f, 0.0f), quat::identity, vec3(1.0f) };

			initializeAnimationComponentAsync(tiran, mesh);
			addRaytracingComponentAsync(tiran, mesh);
		}

		if (auto mesh = loadMeshFromFileAsync(get_asset_path("/resources/assets/Sponza/sponza.obj")))
		{
			auto sponza = world->create_entity("Sponza");
			sponza.add_component<MeshComponent>(mesh);

			TransformComponent& transform_component = sponza.get_component<TransformComponent>();
			transform_component.transform.position = vec3(5.0f, -3.75f, 35.0f);
			transform_component.transform.scale = vec3(0.01f);

			addRaytracingComponentAsync(sponza, mesh);
		}

		{
			Entity physics_sphere = world->create_entity("SpherePX");
			physics_sphere.set_parent(tiran.get_handle());
			physics_sphere.add_component<MeshComponent>(sphereMesh);
			physics::SphereShapeComponent& sphere_component = physics_sphere.add_component<physics::SphereShapeComponent>(1.0f);
			sphere_component.sync_with_joint(tiran.get_data_weakref(), std::string("pelvis"));
			sphere_component.set_attacment_state(true);
			physics_sphere.add_component<physics::DynamicBodyComponent>().use_gravity = false;
		}

		{
			Entity big_physics_sphere = world->create_entity("SpherePX1");
			big_physics_sphere.add_component<MeshComponent>(sphereMesh);
			big_physics_sphere.get_component<TransformComponent>().transform = trs{ vec3(-10.0f, 5.f, 0.0f), quat::identity, vec3(1.5f) };
			big_physics_sphere.add_component<physics::SphereShapeComponent>(1.5f);
			big_physics_sphere.add_component<physics::DynamicBodyComponent>();

			Entity very_big_physics_sphere = world->create_entity("SpherePX2");
			very_big_physics_sphere.add_component<MeshComponent>(sphereMesh);
			very_big_physics_sphere.get_component<TransformComponent>().transform = trs{ vec3(-10.0f, 15.f, 0.0f), quat::identity, vec3(2.5f) };
			very_big_physics_sphere.add_component<physics::SphereShapeComponent>(2.5f);
			very_big_physics_sphere.add_component<physics::DynamicBodyComponent>();

			//physics::JointComponent::BaseDescriptor joint_descriptor;
			//joint_descriptor.enable_collision = false;
			//joint_descriptor.connected_entity = big_physics_sphere.get_data_weakref();
			//joint_descriptor.local_frame = very_big_physics_sphere.get_component<TransformComponent>().transform;
			//joint_descriptor.second_local_frame = big_physics_sphere.get_component<TransformComponent>().transform;
			//physics::D6JointComponent& d6_joint_component = very_big_physics_sphere.add_component<physics::D6JointComponent>(joint_descriptor);
			//d6_joint_component.set_motion(physx::PxD6Axis::eX, physx::PxD6Motion::eLOCKED);
			//d6_joint_component.set_motion(physx::PxD6Axis::eY, physx::PxD6Motion::eLOCKED);
			//d6_joint_component.set_motion(physx::PxD6Axis::eZ, physx::PxD6Motion::eLOCKED);
			//d6_joint_component.set_motion(physx::PxD6Axis::eTWIST, physx::PxD6Motion::eLIMITED);
			//d6_joint_component.set_motion(physx::PxD6Axis::eSWING1, physx::PxD6Motion::eLIMITED);
			//d6_joint_component.set_motion(physx::PxD6Axis::eSWING2, physx::PxD6Motion::eLIMITED);
			//d6_joint_component.set_swing_limit(M_PI / 4.0f, M_PI / 4.0f);
			//d6_joint_component.set_twist_limit(-M_PI / 4.0f, M_PI / 4.0f);
		}

		auto plane = world->create_entity("Platform");
		plane.add_component<physics::PlaneComponent>(vec3(0.f, -5.0, 0.0f));
		plane.add_component<MeshComponent>(groundMesh);
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