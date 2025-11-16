#include "motion_matching/motion/motion_component.h"

#include <ecs/entity.h>
#include <ecs/base_components/transform_component.h>

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<MotionComponent>("MotionComponent")
			.constructor<ref<Entity::EcsData>>();
	}

	MotionComponent::MotionComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		const trs& world_transform = get_entity().get_component<TransformComponent>()->get_world_transform();
		simulation_rotation = world_transform.rotation;
		simulation_position = world_transform.position;
	}

	MotionComponent::~MotionComponent()
	{
	}

	const vec3& MotionComponent::get_current_input() const
	{
		return current_input;
	}

	const vec3& MotionComponent::get_desired_input() const
	{
		return desired_input;
	}

	const vec3& MotionComponent::get_last_input() const
	{
		return last_input;
	}

	const vec3& MotionComponent::get_velocity() const
	{
		return velocity;
	}

	const vec3& MotionComponent::get_last_velocity() const
	{
		return last_velocity;
	}

	void MotionComponent::apply_input(const vec3& input, bool force)
	{
		desired_input += input;
		if (force)
		{
			apply_desired_input();
		}
	}

	void MotionComponent::apply_desired_input()
	{
		last_input = current_input;
		current_input = desired_input;
		desired_input = vec3::zero;
	}

}