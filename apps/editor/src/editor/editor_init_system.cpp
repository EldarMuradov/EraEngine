#include "editor/editor_init_system.h"

#include <core/ecs/input_receiver_component.h>
#include <core/ecs/input_sender_component.h>
#include <core/string.h>
#include <core/ecs/camera_holder_component.h>
#include <core/debug/debug_var.h>

#include <asset/game_asset.h>

#include <animation/animation_clip.h>
#include <animation/animation_clip_utils.h>

#include <rendering/ecs/renderer_holder_root_component.h>

#include <ecs/update_groups.h>
#include <ecs/base_components/transform_component.h>
#include <ecs/rendering/world_renderer.h>
#include <ecs/rendering/scene_rendering.h>
#include <ecs/rendering/mesh_component.h>

#include <physics/body_component.h>
#include <physics/core/physics.h>
#include <physics/shape_component.h>
#include <physics/cct_component.h>
#include <physics/basic_objects.h>
#include <physics/joint.h>
#include <physics/ragdolls/ragdoll_component.h>
#include <physics/physical_animation/physical_animation_component.h>
#include <physics/collisions_holder_root_component.h>
#include <physics/pbd/pbd_cloth_component.h>
#include <physics/vehicles/w4_vehicle_component.h>
#include <physics/physical_animation/dismemberment/ragdoll_dismemberment_component.h>
#include <physics/destructions/destructible_component.h>

#include <audio/audio.h>

#include <terrain/terrain.h>
#include <terrain/grass.h>

#include <animation/skinning.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	static DebugVar<float> sphere_speed = DebugVar<float>("test.sphere_speed", 15000.0f);

	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<EditorInitSystem>("GameInitSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("editor")))
			.method("update", &EditorInitSystem::update)(metadata("update_group", update_types::GAMEPLAY_NORMAL));
	}

	EditorInitSystem::EditorInitSystem(World* _world)
		: System(_world)
	{
	}

	EditorInitSystem::~EditorInitSystem()
	{
	}

	void EditorInitSystem::init()
	{
		using namespace physics;

		{
			mesh_builder builder;

			sphere_render_material = get_default_pbr_material();

			sphere_mesh = make_ref<MultiMesh>();
			builder.pushSphere({ });
			sphere_mesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, sphere_render_material });

			sphere_mesh->mesh = builder.createDXMesh();
			sphere_mesh->load_state = AssetLoadState::LOADED;
		}

		RendererHolderRootComponent* renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);

		camera_entity = world->create_entity("CameraEntity");

		CameraHolderComponent* camera_holder_component = camera_entity.add_component<CameraHolderComponent>();
		camera_holder_component->set_camera_type(CameraHolderComponent::FREE_CAMERA);
		camera_holder_component->set_render_camera(&renderer_holder_rc->camera);

		camera_entity.add_component<InputSenderComponent>()->add_reciever(camera_entity.add_component<InputReceiverComponent>());

		PbrMaterialDesc default_plane_mat_desc;
		default_plane_mat_desc.albedo = get_asset_path("/resources/assets/uv.jpg");
		default_plane_mat_desc.normal = "";
		default_plane_mat_desc.roughness = "";
		default_plane_mat_desc.uv_scale = 15.0f;
		default_plane_mat_desc.metallic_override = 0.35f;
		default_plane_mat_desc.roughness_override = 0.01f;

		auto default_plane_mat = create_pbr_material(default_plane_mat_desc);

		mesh_builder builder;

		ref<MultiMesh> ground_mesh = make_ref<MultiMesh>();
		builder.pushBox({ vec3(0.f), vec3(30.f, 4.f, 30.f) });
		ground_mesh->submeshes.push_back({ builder.endSubmesh(), {}, trs::identity, default_plane_mat });

		Entity plane = world->create_entity("Platform");
		plane.add_component<PlanePhysicsComponent>(CollisionType::TERRAIN, vec3(0.0f, -5.0f, 0.0f));
		plane.add_component<MeshComponent>(ground_mesh);
		plane.get_component<TransformComponent>()->set_world_transform(trs{ vec3(10.0f, -9.0f, 0.0f), quat(vec3(1.0f, 0.0f, 0.0f), deg2rad(0.0f)), vec3(5.0f, 1.0f, 5.0f) });

		ground_mesh->load_state = AssetLoadState::LOADED;
		ground_mesh->mesh = builder.createDXMesh();
	}

	void EditorInitSystem::update(float dt)
	{
		using namespace physics;

		const InputReceiverComponent* input_receiver = camera_entity.get_component<InputReceiverComponent>();

		const UserInput& frame_input = input_receiver->get_frame_input();
		if (frame_input.keyboard[key_code::key_space].press_event)
		{
			Entity sphere = world->create_entity("Sphere");

			const ref<PhysicsMaterial>& material = PhysicsEngine::get_physics_core()->get_default_material();

			SphereShapeComponent* sphere_shape_component = sphere.add_component<SphereShapeComponent>();
			sphere_shape_component->collision_type = CollisionType::COLLISION_0;
			sphere_shape_component->radius = 0.25f;
			sphere_shape_component->material = material;
			sphere.add_component<MeshComponent>(sphere_mesh);

			CameraHolderComponent* camera_holder_component = camera_entity.get_component<CameraHolderComponent>();

			trs camera_world_transform = trs{ camera_holder_component->get_render_camera()->position, camera_holder_component->get_render_camera()->rotation, vec3(0.2f) };
			sphere.get_component<TransformComponent>()->set_world_transform(camera_world_transform);

			DynamicBodyComponent* dynamic_body_component = sphere.add_component<DynamicBodyComponent>();
			dynamic_body_component->ccd.get_for_write() = true;
			dynamic_body_component->mass.get_for_write() = 50.0f;
			dynamic_body_component->simulated.get_for_write() = true;
			dynamic_body_component->use_gravity.get_for_write() = true;
			dynamic_body_component->sleep_threshold.get_for_write() = 0.05f;

			Force& force = dynamic_body_component->forces.emplace_back();
			force.force = (camera_holder_component->get_render_camera()->rotation * quat(vec3(0.0f, 1.0f, 0.0f), M_PI)) * vec3(0.0f, 0.0f, 1.0f) * sphere_speed;
			force.mode = ForceMode::FORCE;
		}
	}

}