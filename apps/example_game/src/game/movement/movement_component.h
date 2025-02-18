#pragma once

#include <core/math.h>

#include <ecs/component.h>

namespace era_engine
{

	class MovementComponent : public Component
	{
	public:
		MovementComponent() = default;
		MovementComponent(ref<Entity::EcsData> _data);

		~MovementComponent() override;

		const vec3& get_current_input() const;
		const vec3& get_desired_input() const;
		const vec3& get_last_input() const;

		const vec3& get_velocity() const;
		const vec3& get_last_velocity() const;

		void apply_input(const vec3& input, bool force = false);

		ERA_VIRTUAL_REFLECT(Component)

	protected:
		void apply_desired_input();

	protected:
		vec3 current_input = vec3::zero;
		vec3 last_input = vec3::zero;
		vec3 desired_input = vec3::zero;

		vec3 velocity = vec3::zero;
		vec3 last_velocity = vec3::zero;

		friend class MovementSystem;
	};

}