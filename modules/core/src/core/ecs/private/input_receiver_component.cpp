#include "core/ecs/input_receiver_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<InputReceiverComponent>("InputReceiverComponent")
			.constructor<ref<Entity::EcsData>>();
	}


	InputReceiverComponent::InputReceiverComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	InputReceiverComponent::~InputReceiverComponent()
	{
	}

	const vec3& InputReceiverComponent::get_current_input() const
	{
		return current_input;
	}

	bool era_engine::InputReceiverComponent::is_active() const
	{
		return active;
	}

	const UserInput& InputReceiverComponent::get_frame_input() const
	{
		return frame_input;
	}

}