#include "physics/physical_animation/physical_animation_utils.h"

namespace era_engine::physics
{

	vec3 PhysicalAnimationUtils::calculate_delta_rotation_time_derivative(const quat& delta_rotation, float dt)
	{
		float delta_angle = 0.0f;
		vec3 delta_axis = vec3::zero;
		get_axis_rotation(delta_rotation, delta_axis, delta_angle);

		const vec3 desired_angular_velocity = normalize(delta_axis) * delta_angle / dt;
		return desired_angular_velocity;
	}

	bool PhysicalAnimationUtils::is_low_limb(PhysicalAnimationLimbComponent::Type type)
	{
		return type == PhysicalAnimationLimbComponent::Type::LEG ||
			type == PhysicalAnimationLimbComponent::Type::CALF ||
			type == PhysicalAnimationLimbComponent::Type::FOOT;
	}
}