#pragma once

#include "motion_matching_api.h"

#include <core/math.h>

#include <ecs/component.h>

namespace era_engine
{
	class ERA_MOTION_MATCHING_API TrajectoryComponent final : public Component
	{
	public:
		TrajectoryComponent() = default;
		TrajectoryComponent(ref<Entity::EcsData> _data);

		~TrajectoryComponent() override;

		ERA_VIRTUAL_REFLECT(Component)

	public:
		std::vector<vec3> trajectory_desired_velocities;
		std::vector<quat> trajectory_desired_rotations;

		std::vector<vec3> trajectory_positions;
		std::vector<quat> trajectory_rotations;

		std::vector<vec3> trajectory_velocities;
		std::vector<vec3> trajectory_accelerations;

		std::vector<vec3> trajectory_angular_velocities;

		size_t number_of_trajectories = 5;

		std::vector<float> time_offsets;
	};
}