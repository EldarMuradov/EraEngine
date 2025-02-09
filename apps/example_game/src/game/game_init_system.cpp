#include "game/game_init_system.h"

#include <core/ecs/input_reciever_component.h>
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
			.constructor<World*>()(policy::ctor::as_raw_ptr)
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

		Entity camera_entity = world->create_entity("Entity1");
		camera_entity.add_component<CameraHolderComponent>().add_component<InputRecieverComponent>().add_component<MovementComponent>();
		camera_entity.get_component<CameraHolderComponent>().set_camera_type(CameraHolderComponent::ATTACHED_TO_TRS);
		camera_entity.get_component<CameraHolderComponent>().set_render_camera(&renderer_holder_rc->camera);

		Entity entity2 = world->create_entity("Entity2");

#if 0
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

		if (auto mesh = loadAnimatedMeshFromFileAsync(get_asset_path("/resources/assets/veribot/source/VERIBOT_final.fbx")))
		{
			auto& en = world->create_entity("Veribot")
				.add_component<animation::AnimationComponent>()
				.add_component<MeshComponent>(mesh);

			TransformComponent& transform_component = en.get_component<TransformComponent>();
			transform_component.type = TransformComponent::DYNAMIC;
			transform_component.transform.position = vec3(5.0f);

			initializeAnimationComponentAsync(en, mesh);
			addRaytracingComponentAsync(en, mesh);
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

		auto chainMesh = make_ref<multi_mesh>();

		groundMesh->mesh =
			boxMesh->mesh =
			sphereMesh->mesh =
			chainMesh->mesh =
			builder.createDXMesh();

#endif
	}

	void GameInitSystem::update(float dt)
	{
		
	}

}