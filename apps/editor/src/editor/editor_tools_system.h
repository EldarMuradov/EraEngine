#pragma once

#include <ecs/system.h>
#include <ecs/entity.h>

#include <core/math.h>

namespace era_engine
{
	class RendererHolderRootComponent;

	class EditorToolsSystem final : public System
	{
	public:
		EditorToolsSystem(World* _world);
		~EditorToolsSystem();

		void init() override;
		void update(float dt) override;

		ERA_VIRTUAL_REFLECT(System)

	private:
		void draw_creation_popup();
		void draw_world();

		void draw_childs(const Entity& entity);

		void update_selected_entity_ui_rotation();

		void set_selected_entity(const Entity& entity);

	private:
		RendererHolderRootComponent* renderer_holder_rc = nullptr;

		Entity selected_entity = Entity::Null;
		vec3 selected_entity_euler_rotation = vec3::zero;
	};
}