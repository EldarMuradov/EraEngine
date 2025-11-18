#include "physics/ragdolls/ragdoll_component.h"

#include <rttr/registration>

namespace era_engine::physics
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<RagdollComponent>("RagdollComponent")
			.constructor<>();

		registration::class_<RagdollLimbComponent>("RagdollLimbComponent")
			.constructor<>();
	}

	RagdollLimbComponent::RagdollLimbComponent(ref<Entity::EcsData> _data, uint32 _joint_id /*= INVALID_JOINT*/)
		: Component(_data), joint_id(_joint_id)
	{
		simulated.set_component(ComponentPtr(this));
	}

	RagdollLimbComponent::~RagdollLimbComponent()
	{
	}

	RagdollComponent::RagdollComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		simulated.set_component(ComponentPtr(this));
	}

	RagdollComponent::~RagdollComponent()
	{
	}

}