#include "physics/destructions/destructible_component.h"

#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<DestructibleComponent>("DestructibleComponent")
			.constructor<>();
	}

	DestructibleComponent::DestructibleComponent(ref<Entity::EcsData> _data, uint32 _health, bool _is_root)
		: Component(_data)
		, health(_health)
		, is_root(_is_root)
	{
	}

	DestructibleComponent::~DestructibleComponent()
	{
		const uint32 actor_count = NvBlastFamilyGetActorCount(family, nullptr);

		const bool success = NvBlastActorDeactivate(blast_actor, nullptr);
		ASSERT(success);

		if (actor_count == 1)
		{
			free(family);
			family = nullptr;
		}
	}

}