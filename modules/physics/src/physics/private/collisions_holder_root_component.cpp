#include "physics/collisions_holder_root_component.h"

#include <core/math.h>

#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<CollisionsHolderRootComponent>("CollisionsHolderRootComponent")
			.constructor<>();
	}

	CollisionsHolderRootComponent::CollisionsHolderRootComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	void CollisionsHolderRootComponent::set_collision_filter(uint32 type, uint32 types_to_collide_with, bool collide)
	{
        ASSERT(type != 0);
        ASSERT((type & (type - 1)) == 0);
        const size_t type_index = log2(type);

        ASSERT(types_to_collide_with != 0);

        // For each type.
        for (uint32 i = 0; i < TYPES_COUNT; ++i)
        {
            const uint32 type_to_collide_with = (1u << i);

            if ((types_to_collide_with & type_to_collide_with) == type_to_collide_with)
            {
                const uint32 type_to_collide_with_index = log2(type_to_collide_with);

                if (collide)
                {
                    collision_map[type_index] |= type_to_collide_with;
                    collision_map[type_to_collide_with_index] |= type;
                }
                else
                {
                    collision_map[type_index] &= ~type_to_collide_with;
                    collision_map[type_to_collide_with_index] &= ~type;
                }

                ASSERT(check_collision(type, type_to_collide_with) == collide);
            }
        }
	}

	uint32 CollisionsHolderRootComponent::get_collision_filter(uint32 types) const
	{
        uint32 result = 0;

        for (uint32 i = 0; i < TYPES_COUNT; ++i)
        {
            uint32 mask = (1 << i);
            if ((types & mask) == mask)
            {
                result |= collision_map[i];
            }
        }

        return result;
    }

	bool CollisionsHolderRootComponent::check_collision(uint32 first_type, uint32 second_type) const
	{
        const uint32 collision_filter = get_collision_filter(first_type);

        return (collision_filter & second_type) > 0;
    }
}