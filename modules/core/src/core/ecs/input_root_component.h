#pragma once

#include "core_api.h"

#include "core/input.h"
#include "core/math.h"

#include "ecs/component.h"

namespace era_engine
{

	class ERA_CORE_API InputRootComponent final : public Component
	{
	public:
		InputRootComponent() = default;
		InputRootComponent(ref<Entity::EcsData> _data);

		~InputRootComponent() override;

		const vec3& get_current_input() const;
		const vec3& get_desired_input() const;
		const vec3& get_last_input() const;

		const UserInput& get_frame_input() const;

		ERA_VIRTUAL_REFLECT(Component)

	private:
		UserInput frame_input;

		vec3 current_input;
		vec3 last_input;
		vec3 desired_input;

		friend class InputSystem;
	};

}