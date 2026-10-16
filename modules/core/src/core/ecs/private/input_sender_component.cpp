#include "core/ecs/input_sender_component.h"
#include "core/ecs/input_receiver_component.h"

#include "ecs/serialization/component_registry.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<InputSenderComponent>("InputSenderComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("recievers", &InputSenderComponent::get_reciever_entities, &InputSenderComponent::set_reciever_entities);
		ComponentRegistry::register_component<InputSenderComponent>();
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

	void InputSenderComponent::add_reciever(InputReceiverComponent* reciever)
	{
		recievers.push_back(reciever);
	}

	void InputSenderComponent::remove_reciever(InputReceiverComponent* reciever)
	{
		std::erase(recievers, reciever);
	}

	std::vector<EntityPtr> InputSenderComponent::get_reciever_entities() const
	{
		std::vector<EntityPtr> result;
		result.reserve(recievers.size());
		for (const InputReceiverComponent* reciever : recievers)
		{
			result.emplace_back(reciever->get_entity());
		}
		return result;
	}

	void InputSenderComponent::set_reciever_entities(std::vector<EntityPtr> entities)
	{
		pending_reciever_entities = std::move(entities);
	}

	void InputSenderComponent::on_deserialized()
	{
		for (const EntityPtr& entity_ptr : pending_reciever_entities)
		{
			Entity entity = entity_ptr.get();
			if (!entity.is_valid())
			{
				continue;
			}

			InputReceiverComponent* reciever = entity.get_component_if_exists<InputReceiverComponent>();
			if (reciever != nullptr && std::find(recievers.begin(), recievers.end(), reciever) == recievers.end())
			{
				add_reciever(reciever);
			}
		}
		pending_reciever_entities.clear();
	}

	void InputSenderComponent::notify_input()
	{
		for (InputReceiverComponent* reciever : recievers)
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