#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"
#include "physics/collision_types.h"

#include <ecs/component.h>
#include <ecs/observable_member.h>

namespace era_engine::physics
{
	class ERA_PHYSICS_API CollisionsHolderRootComponent final : public Component
	{
	public:
		CollisionsHolderRootComponent() = default;
		CollisionsHolderRootComponent(ref<Entity::EcsData> _data);

		void set_collision_filter(uint32 type, uint32 types_to_collide_with, bool collide);
		uint32 get_collision_filter(uint32 types) const;

		bool check_collision(uint32 first_type, uint32 second_type) const;

	private:
		std::array<uint32, TYPES_COUNT> collision_map = {};
	};
}