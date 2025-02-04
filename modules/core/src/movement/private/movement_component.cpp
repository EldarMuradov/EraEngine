#include "movement/movement_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<MovementComponent>("MovementComponent")
			.constructor<ref<Entity::EcsData>>();
	}


	MovementComponent::MovementComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	MovementComponent::~MovementComponent()
	{
	}

	const vec3& MovementComponent::get_current_input() const
	{
		return current_input;
	}

	const vec3& MovementComponent::get_desired_input() const
	{
		return desired_input;
	}

	const vec3& MovementComponent::get_last_input() const
	{
		return last_input;
	}

	const vec3& MovementComponent::get_velocity() const
	{
		return velocity;
	}

	const vec3& MovementComponent::get_last_velocity() const
	{
		return last_velocity;
	}

	void MovementComponent::apply_input(const vec3& input, bool force)
	{
		desired_input += input;
		if (force)
		{
			apply_desired_input();
		}
	}

	void MovementComponent::apply_desired_input()
	{
		last_input = current_input;
		current_input = desired_input;
		desired_input = vec3::zero;
	}

}