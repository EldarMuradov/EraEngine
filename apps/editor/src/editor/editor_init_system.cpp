#include "editor/editor_init_system.h"

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

		registration::class_<EditorInitSystem>("GameInitSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr)
			.method("update", &EditorInitSystem::update)(metadata("update_group", update_types::BEGIN));
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
		RendererHolderRootComponent* renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);

		Entity camera_entity = world->create_entity("CameraEntity");
		camera_entity.add_component<CameraHolderComponent>()
					 .add_component<InputRecieverComponent>()
					 .add_component<InputSenderComponent>();

		CameraHolderComponent& camera_holder_component = camera_entity.get_component<CameraHolderComponent>();
		camera_holder_component.set_camera_type(CameraHolderComponent::FREE_CAMERA);
		camera_holder_component.set_render_camera(&renderer_holder_rc->camera);

		camera_entity.get_component<InputSenderComponent>().add_reciever(camera_entity.get_component_if_exists<InputRecieverComponent>());
	}

	void EditorInitSystem::update(float dt)
	{
		
	}

}