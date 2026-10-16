#pragma once

#include "core_api.h"

#include "core/input.h"
#include "core/math.h"

#include "ecs/component.h"

namespace era_engine
{
	class InputReceiverComponent;

	class ERA_CORE_API InputSenderComponent final : public Component
	{
	public:
		InputSenderComponent() = default;
		InputSenderComponent(ref<Entity::EcsData> _data);

		~InputSenderComponent() override;

		const vec3& get_current_input() const;
		const vec3& get_last_input() const;

		const UserInput& get_frame_input() const;

		void add_reciever(InputReceiverComponent* reciever);
		void remove_reciever(InputReceiverComponent* reciever);

		std::vector<EntityPtr> get_reciever_entities() const;
		void set_reciever_entities(std::vector<EntityPtr> entities);

		void on_deserialized() override;

		ERA_VIRTUAL_REFLECT(Component)

	private:
		void notify_input();

	private:
		UserInput frame_input{};

		vec3 current_input = vec3::zero;
		vec3 last_input = vec3::zero;

		std::vector<InputReceiverComponent*> recievers;
		std::vector<EntityPtr> pending_reciever_entities;

		friend class InputSystem;
	};

}