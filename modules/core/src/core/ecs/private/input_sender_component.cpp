#include "core/ecs/input_sender_component.h"
#include "core/ecs/input_reciever_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<InputSenderComponent>("InputSenderComponent")
			.constructor<ref<Entity::EcsData>>();
	}


	InputSenderComponent::InputSenderComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	InputSenderComponent::~InputSenderComponent()
	{
	}

	const vec3& InputSenderComponent::get_current_input() const
	{
		return current_input;
	}

	const vec3& InputSenderComponent::get_last_input() const
	{
		return last_input;
	}

	const UserInput& InputSenderComponent::get_frame_input() const
	{
		return frame_input;
	}

	void InputSenderComponent::add_reciever(InputRecieverComponent* reciever)
	{
		recievers.push_back(reciever);
	}

	void InputSenderComponent::remove_reciever(InputRecieverComponent* reciever)
	{
		std::erase(recievers, reciever);
	}

	void InputSenderComponent::notify_input()
	{
		for (InputRecieverComponent* reciever : recievers)
		{
			if (!reciever->is_active())
			{
				continue;
			}

			reciever->frame_input = frame_input;
			reciever->current_input = current_input;
		}
	}

}