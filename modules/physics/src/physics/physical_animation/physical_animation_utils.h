#pragma once

#include "physics_api.h"
#include "physics/physical_animation/physical_animation_component.h"

#include <core/math.h>

namespace era_engine::physics
{
	class ERA_PHYSICS_API PhysicalAnimationUtils final
	{
	public:
		PhysicalAnimationUtils() = delete;

		static vec3 calculate_delta_rotation_time_derivative(const quat& delta_rotation,
			float dt);

		static bool is_low_limb(PhysicalAnimationLimbComponent::Type type);
	};
}