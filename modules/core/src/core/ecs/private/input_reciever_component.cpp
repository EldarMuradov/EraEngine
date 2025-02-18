#include "core/ecs/input_reciever_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<InputRecieverComponent>("InputRecieverComponent")
			.constructor<ref<Entity::EcsData>>();
	}


	InputRecieverComponent::InputRecieverComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	InputRecieverComponent::~InputRecieverComponent()
	{
	}

	const vec3& InputRecieverComponent::get_current_input() const
	{
		return current_input;
	}

	bool era_engine::InputRecieverComponent::is_active() const
	{
		return active;
	}

	const UserInput& InputRecieverComponent::get_frame_input() const
	{
		return frame_input;
	}

}