#include "core/ecs/input_root_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<InputRootComponent>("InputRootComponent")
			.constructor<ref<Entity::EcsData>>();
	}


	InputRootComponent::InputRootComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	InputRootComponent::~InputRootComponent()
	{
	}

	const vec3& InputRootComponent::get_current_input() const
	{
		return current_input;
	}

	const vec3& InputRootComponent::get_last_input() const
	{
		return last_input;
	}

	const UserInput& InputRootComponent::get_frame_input() const
	{
		return frame_input;
	}

}