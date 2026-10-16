#include "motion_matching/trajectory/trajectory_component.h"

#include <ecs/serialization/component_registry.h>

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<TrajectoryComponent>("TrajectoryComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("number_of_trajectories", &TrajectoryComponent::number_of_trajectories)
			.property("time_offsets", &TrajectoryComponent::time_offsets);
		ComponentRegistry::register_component<TrajectoryComponent>();
	}

	TrajectoryComponent::TrajectoryComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		trajectory_desired_velocities.assign(number_of_trajectories, vec3(0.0f));
		trajectory_desired_rotations.assign(number_of_trajectories, quat::identity);
		trajectory_positions.assign(number_of_trajectories, vec3(0.0f));
		trajectory_rotations.assign(number_of_trajectories, quat::identity);
		trajectory_velocities.assign(number_of_trajectories, vec3(0.0f));
		trajectory_accelerations.assign(number_of_trajectories, vec3(0.0f));
		trajectory_angular_velocities.assign(number_of_trajectories, vec3(0.0f));

		time_offsets.assign(number_of_trajectories, 0.0f);
	}

	TrajectoryComponent::~TrajectoryComponent()
	{
	}

	void TrajectoryComponent::on_deserialized()
	{
		trajectory_desired_velocities.resize(number_of_trajectories, vec3(0.0f));
		trajectory_desired_rotations.resize(number_of_trajectories, quat::identity);
		trajectory_positions.resize(number_of_trajectories, vec3(0.0f));
		trajectory_rotations.resize(number_of_trajectories, quat::identity);
		trajectory_velocities.resize(number_of_trajectories, vec3(0.0f));
		trajectory_accelerations.resize(number_of_trajectories, vec3(0.0f));
		trajectory_angular_velocities.resize(number_of_trajectories, vec3(0.0f));
		time_offsets.resize(number_of_trajectories, 0.0f);
	}

}