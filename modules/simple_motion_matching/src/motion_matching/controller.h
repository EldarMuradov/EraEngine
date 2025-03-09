#pragma once

#include "motion_matching_api.h"

#include "motion_matching/array.h"
#include "motion_matching/common.h"
#include "motion_matching/vec.h"
#include "motion_matching/quat.h"

#include "ecs/component.h"

namespace era_engine
{

	class ERA_MOTION_MATCHING_API MotionMatchingControllerComponent final : public Component
	{
	public:
		MotionMatchingControllerComponent() = default;
		MotionMatchingControllerComponent(ref<Entity::EcsData> _data);

		~MotionMatchingControllerComponent() override;

		int frame_index = -1;
		float inertialize_blending_halflife = 0.1f;

		array1d<Vec3> curr_bone_positions;
		array1d<Vec3> curr_bone_velocities;
		array1d<Quat> curr_bone_rotations;
		array1d<Vec3> curr_bone_angular_velocities;
		array1d<bool> curr_bone_contacts;

		array1d<Vec3> trns_bone_positions;
		array1d<Vec3> trns_bone_velocities;
		array1d<Quat> trns_bone_rotations;
		array1d<Vec3> trns_bone_angular_velocities;
		array1d<bool> trns_bone_contacts;

		array1d<Vec3> bone_positions;
		array1d<Vec3> bone_velocities;
		array1d<Quat> bone_rotations;
		array1d<Vec3> bone_angular_velocities;

		array1d<Vec3> bone_offset_positions;
		array1d<Vec3> bone_offset_velocities;
		array1d<Quat> bone_offset_rotations;
		array1d<Vec3> bone_offset_angular_velocities;

		array1d<Vec3> global_bone_positions;
		array1d<Vec3> global_bone_velocities;
		array1d<Quat> global_bone_rotations;
		array1d<Vec3> global_bone_angular_velocities;
		array1d<bool> global_bone_computed;

		Vec3 transition_src_position = Vec3();
		Quat transition_src_rotation = Quat();
		Vec3 transition_dst_position = Vec3();
		Quat transition_dst_rotation = Quat();

		float search_time = 0.1f;
		float search_timer = search_time;
		float force_search_timer = search_time;

		Vec3 desired_velocity = Vec3();
		Vec3 desired_velocity_change_curr = Vec3();
		Vec3 desired_velocity_change_prev = Vec3();
		float desired_velocity_change_threshold = 50.0;

		Quat desired_rotation = Quat();
		Vec3 desired_rotation_change_curr = Vec3();
		Vec3 desired_rotation_change_prev = Vec3();
		float desired_rotation_change_threshold = 50.0;

		float desired_gait = 0.0f;
		float desired_gait_velocity = 0.0f;

		Vec3 simulation_position = Vec3();
		Vec3 simulation_velocity = Vec3();
		Vec3 simulation_acceleration = Vec3();
		Quat simulation_rotation = Quat();
		Vec3 simulation_angular_velocity = Vec3();

		float simulation_velocity_halflife = 0.27f;
		float simulation_rotation_halflife = 0.27f;

		// All speeds in m/s
		float simulation_run_fwrd_speed = 4.0f;
		float simulation_run_side_speed = 3.0f;
		float simulation_run_back_speed = 2.5f;

		float simulation_walk_fwrd_speed = 1.75f;
		float simulation_walk_side_speed = 1.5f;
		float simulation_walk_back_speed = 1.25f;

		array1d<Vec3> trajectory_desired_velocities;
		array1d<Quat> trajectory_desired_rotations;
		array1d<Vec3> trajectory_positions;
		array1d<Vec3> trajectory_velocities;
		array1d<Vec3> trajectory_accelerations;
		array1d<Quat> trajectory_rotations;
		array1d<Vec3> trajectory_angular_velocities;

		// Synchronization

		float synchronization_data_factor = 1.0f;

		// Adjustment

		bool adjustment_by_velocity_enabled = true;
		float adjustment_position_halflife = 0.1f;
		float adjustment_rotation_halflife = 0.2f;
		float adjustment_position_max_ratio = 0.5f;
		float adjustment_rotation_max_ratio = 0.5f;

		// Clamping

		float clamping_max_distance = 0.15f;
		float clamping_max_angle = 0.5f * PIf;

		// IK

		float ik_max_length_buffer = 0.015f;
		float ik_foot_height = 0.02f;
		float ik_toe_length = 0.15f;
		float ik_unlock_radius = 0.2f;
		float ik_blending_halflife = 0.1f;

		array1d<int> contact_bones;

		array1d<bool> contact_states;
		array1d<bool> contact_locks;
		array1d<Vec3> contact_positions;
		array1d<Vec3> contact_velocities;
		array1d<Vec3> contact_points;
		array1d<Vec3> contact_targets;
		array1d<Vec3> contact_offset_positions;
		array1d<Vec3> contact_offset_velocities;

		array1d<Vec3> adjusted_bone_positions;
		array1d<Quat> adjusted_bone_rotations;

		ERA_VIRTUAL_REFLECT(Component)
	};

}