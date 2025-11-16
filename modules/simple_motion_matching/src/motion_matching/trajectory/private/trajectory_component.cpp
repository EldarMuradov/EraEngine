#include "motion_matching/trajectory/trajectory_component.h"

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<TrajectoryComponent>("TrajectoryComponent")
			.constructor<ref<Entity::EcsData>>();
	}

	TrajectoryComponent::TrajectoryComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		trajectory_desired_velocities.resize(4);
		trajectory_desired_rotations.resize(4);
		trajectory_positions.resize(4);
		trajectory_velocities.resize(4);
		trajectory_accelerations.resize(4);
		trajectory_rotations.resize(4);
		trajectory_angular_velocities.resize(4);
	}

	TrajectoryComponent::~TrajectoryComponent()
	{
	}

}