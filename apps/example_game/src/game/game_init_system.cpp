#include "game/game_init_system.h"
#include "game/physics/gameplay_physics_types.h"

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
#include <physics/ragdoll_component.h>
#include <physics/physical_animation/physical_animation_component.h>

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
		using namespace animation;
		using namespace physics;

		RendererHolderRootComponent* renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);

		Entity camera_entity = world->create_entity("CameraEntity");
		CameraHolderComponent* camera_holder_component = camera_entity.add_component<CameraHolderComponent>();

		camera_entity.add_component<InputSenderComponent>()->add_reciever(camera_entity.add_component<InputRecieverComponent>());
		camera_entity.add_component<MovementComponent>();

		camera_holder_component->set_camera_type(CameraHolderComponent::FREE_CAMERA);
		camera_holder_component->set_render_camera(&renderer_holder_rc->camera);

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

		if (auto mesh = loadAnimatedMeshFromFileAsync(get_asset_path("/resources/assets/springtrap/source/Springtrap.fbx"), 
			mesh_creation_flags_unreal_animated_asset))
		{
			tiran = world->create_entity("Tiran");

			AnimationComponent* animation_component = tiran.add_component<AnimationComponent>();
			//animation_component->play = false;
			animation_component->update_skeleton = false;
			animation_component->enable_root_motion = false;

			SkeletonComponent* skeleton_component = tiran.add_component<SkeletonComponent>();
			tiran.add_component<MeshComponent>(mesh);

			TransformComponent* transform_component = tiran.get_component<TransformComponent>();
			transform_component->set_world_transform(trs{vec3(-10.0f, -2.0f, 0.0f), quat::identity, vec3(1.0f)});

			mesh->loadJob.wait_for_completion();

			animation_component->initialize(mesh->animation_skeleton.clips, 74);
			skeleton_component->skeleton = &mesh->skeleton;
			skeleton_component->draw_sceleton = true;

			const Skeleton* skeleton = skeleton_component->skeleton;

			RagdollJointIds joint_init_ids;
			joint_init_ids.head_end_idx = skeleton->name_to_joint_id.at("joint_HeadA_01");
			joint_init_ids.head_idx = skeleton->name_to_joint_id.at("joint_HeadA_01");
			joint_init_ids.neck_idx = skeleton->name_to_joint_id.at("joint_NeckA_01");

			joint_init_ids.spine_03_idx = skeleton->name_to_joint_id.at("joint_TorsoC_01");
			joint_init_ids.spine_02_idx = skeleton->name_to_joint_id.at("joint_TorsoB_01");
			joint_init_ids.spine_01_idx = skeleton->name_to_joint_id.at("joint_TorsoA_01");
			joint_init_ids.pelvis_idx = skeleton->name_to_joint_id.at("joint_Pelvis_01");

			joint_init_ids.root_idx = skeleton->name_to_joint_id.at("joint_Char");
			joint_init_ids.attachment_idx = skeleton->name_to_joint_id.at("joint_Pelvis_01");

			joint_init_ids.thigh_l_idx = skeleton->name_to_joint_id.at("joint_HipLT_01");
			joint_init_ids.calf_l_idx = skeleton->name_to_joint_id.at("joint_KneeLT_01");
			joint_init_ids.foot_l_idx = skeleton->name_to_joint_id.at("joint_FootLT_01");
			joint_init_ids.foot_end_l_idx = skeleton->name_to_joint_id.at("joint_ToeLT_01");

			joint_init_ids.thigh_r_idx = skeleton->name_to_joint_id.at("joint_HipRT_01");
			joint_init_ids.calf_r_idx = skeleton->name_to_joint_id.at("joint_KneeRT_01");
			joint_init_ids.foot_r_idx = skeleton->name_to_joint_id.at("joint_FootRT_01");
			joint_init_ids.foot_end_r_idx = skeleton->name_to_joint_id.at("joint_ToeRT_01");

			joint_init_ids.upperarm_l_idx = skeleton->name_to_joint_id.at("joint_ShoulderLT_01");
			joint_init_ids.lowerarm_l_idx = skeleton->name_to_joint_id.at("joint_ElbowLT_01");
			joint_init_ids.hand_l_idx = skeleton->name_to_joint_id.at("joint_HandLT_01");
			joint_init_ids.hand_end_l_idx = skeleton->name_to_joint_id.at("joint_FingerBLT_01");

			joint_init_ids.upperarm_r_idx = skeleton->name_to_joint_id.at("joint_ShoulderRT_01");
			joint_init_ids.lowerarm_r_idx = skeleton->name_to_joint_id.at("joint_ElbowRT_01");
			joint_init_ids.hand_r_idx = skeleton->name_to_joint_id.at("joint_HandRT_01");
			joint_init_ids.hand_end_r_idx = skeleton->name_to_joint_id.at("joint_FingerBRT_01");

			RagdollSettings settings;
			settings.head_radius = 0.15f;

			settings.head_end_joint_adjastment = vec3(0.0f, 0.2f, 0.0f);
			settings.head_joint_adjastment = vec3(0.0f, 0.05f, 0.0f);
			settings.neck_joint_adjastment = vec3(0.0f, 0.05f, 0.0f);
			settings.thorax_joint_adjastment = vec3(0.0f, 0.18f, 0.0f);
			settings.abdomen_joint_adjastment = vec3(0.0f, 0.05f, 0.0f);
			settings.pelvis_joint_adjastment = vec3(0.0f, -0.05f, 0.0f);

			settings.upper_body_height_modifier = 0.5f;
			settings.upper_body_radius_modifier = 0.6f;
			settings.middle_body_height_modifier = 0.4f;
			settings.middle_body_radius_modifier = 0.8f;
			settings.lower_body_height_modifier = 1.2f;
			settings.lower_body_radius_modifier = 1.4f;

			RagdollComponent* ragdoll_component = tiran.add_component<RagdollComponent>();
			ragdoll_component->simulated = true;
			//PhysicalAnimationComponent* ragdoll_component = tiran.add_component<PhysicalAnimationComponent>();
			ragdoll_component->joint_init_ids = joint_init_ids;
			ragdoll_component->settings = settings;
		}

		if (auto mesh = loadMeshFromFileAsync(get_asset_path("/resources/assets/Sponza/sponza.obj"), mesh_creation_flags_unreal_asset))
		{
			auto sponza = world->create_entity("Sponza");
			sponza.add_component<MeshComponent>(mesh);

			TransformComponent* transform_component = sponza.get_component<TransformComponent>();
			transform_component->set_world_position(vec3(5.0f, -3.75f, 35.0f));

			//addRaytracingComponentAsync(sponza, mesh);
		}

		auto plane = world->create_entity("Platform");
		plane.add_component<physics::PlaneComponent>(CollisionType::TERRAIN, vec3(0.f, -5.0, 0.0f));
		plane.add_component<MeshComponent>(groundMesh);
		plane.get_component<TransformComponent>()->set_world_transform(trs{vec3(10, -9.f, 0.f), quat(vec3(1.f, 0.f, 0.f), deg2rad(0.f)), vec3(5.0f, 1.0f, 5.0f)});
		
		groundMesh->mesh =
			boxMesh->mesh =
			sphereMesh->mesh =
			builder.createDXMesh();
	}

	void GameInitSystem::update(float dt)
	{
		
	}

}