#include <gtest/gtest.h>

#include <ik/full_body_ik_solver.h>
#include <animation/skeleton.h>

#include <cstdio>

namespace
{
	using namespace era_engine;
	using namespace era_engine::animation;

	enum Joint : uint32
	{
		HIPS, SPINE, SPINE1, SPINE2,
		L_CLAVICLE, L_ARM, L_FOREARM, L_HAND,
		R_CLAVICLE, R_ARM, R_FOREARM, R_HAND,
		NECK, HEAD,
		JOINTS_COUNT
	};

	struct TestRig
	{
		Skeleton skeleton;
		std::vector<vec3> offsets;
		uint32 bone_children[JOINTS_COUNT] = { SPINE, SPINE1, SPINE2, NECK, L_ARM, L_FOREARM, L_HAND, INVALID_JOINT, R_ARM, R_FOREARM, R_HAND, INVALID_JOINT, HEAD, INVALID_JOINT };

		TestRig()
		{
			const uint32 parents[JOINTS_COUNT] = { INVALID_JOINT, HIPS, SPINE, SPINE1, SPINE2, L_CLAVICLE, L_ARM, L_FOREARM, SPINE2, R_CLAVICLE, R_ARM, R_FOREARM, SPINE2, NECK };
			offsets = {
				vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.1f, 0.0f), vec3(0.0f, 0.12f, 0.0f), vec3(0.0f, 0.14f, 0.0f),
				vec3(0.06f, 0.1f, 0.0f), vec3(0.12f, 0.0f, 0.0f), vec3(0.27f, 0.0f, 0.0f), vec3(0.25f, 0.0f, 0.0f),
				vec3(-0.06f, 0.1f, 0.0f), vec3(-0.12f, 0.0f, 0.0f), vec3(-0.27f, 0.0f, 0.0f), vec3(-0.25f, 0.0f, 0.0f),
				vec3(0.0f, 0.15f, 0.0f), vec3(0.0f, 0.1f, 0.0f) };

			skeleton.joints.resize(JOINTS_COUNT);
			skeleton.default_local_transforms.resize(JOINTS_COUNT);
			for (uint32 i = 0; i < JOINTS_COUNT; ++i)
			{
				skeleton.joints[i].parent_id = parents[i];
				skeleton.default_local_transforms[i].set_transform(trs(offsets[i], quat::identity, vec3(1.0f)));
			}
		}

		// Walk-like animation: hips bob and yaw, spine sway, arms swing.
		SkeletonPose animate(float time) const
		{
			SkeletonPose pose(JOINTS_COUNT);
			for (uint32 i = 0; i < JOINTS_COUNT; ++i)
			{
				pose.set_joint_translation(offsets[i], i);
				pose.set_joint_rotation(quat::identity, i);
				pose.set_joint_scale(vec3(1.0f), i);
			}

			const float phase = time * 2.0f * M_PI;
			pose.set_joint_translation(offsets[HIPS] + vec3(0.0f, 0.03f * std::sin(2.0f * phase), 0.0f), HIPS);
			pose.set_joint_rotation(quat(vec3(0.0f, 1.0f, 0.0f), deg2rad(8.0f) * std::sin(phase)), HIPS);
			pose.set_joint_rotation(quat(vec3(0.0f, 1.0f, 0.0f), deg2rad(-5.0f) * std::sin(phase)), SPINE1);

			const float swing = deg2rad(30.0f) * std::sin(phase);
			pose.set_joint_rotation(quat(vec3(1.0f, 0.0f, 0.0f), swing) * quat(vec3(0.0f, 0.0f, 1.0f), deg2rad(-70.0f)), L_ARM);
			pose.set_joint_rotation(quat(vec3(1.0f, 0.0f, 0.0f), -swing) * quat(vec3(0.0f, 0.0f, 1.0f), deg2rad(70.0f)), R_ARM);
			// Elbows flex forward, like a human arm.
			pose.set_joint_rotation(quat(vec3(0.0f, 1.0f, 0.0f), deg2rad(-20.0f)), L_FOREARM);
			pose.set_joint_rotation(quat(vec3(0.0f, 1.0f, 0.0f), deg2rad(20.0f)), R_FOREARM);
			return pose;
		}
	};

	float rotation_angle(const quat& a, const quat& b)
	{
		const float d = std::abs(dot(a.v, b.v) + a.w * b.w);
		return 2.0f * std::acos(std::min(d, 1.0f));
	}

	float wrap_angle(float angle)
	{
		if (angle > M_PI)
		{
			return angle - 2.0f * M_PI;
		}
		if (angle < -M_PI)
		{
			return angle + 2.0f * M_PI;
		}
		return angle;
	}

	// Twist of the solved local rotation around its bone relative to the animated one, both from the swing-twist decomposition from the identity rest.
	float twist_from_animation(const quat& solved, const quat& animated, const vec3& axis)
	{
		return wrap_angle(2.0f * std::atan2(dot(solved.v, axis), solved.w) - 2.0f * std::atan2(dot(animated.v, axis), animated.w));
	}

	struct ScenarioResult
	{
		float worst_jerk_deg = 0.0f;
		float worst_twist_deg = 0.0f;
		float worst_clavicle_swing_deg = 0.0f;
		float max_error = 0.0f;
	};

	struct Scenario
	{
		const char* name;
		float root_rotation_mobility = 0.0f;
		float root_position_mobility = 0.0f;
		float walk_speed = 0.0f; // Character moves through the targets.
		vec3 left_target;
		vec3 right_target;
		float target_circle_radius = 0.0f;
		vec3 target_circle_axis = vec3(0.0f, 0.0f, 1.0f);
	};

	// Max per frame change of the solved local rotation deltas (a spike = jerk), beyond the same metric of the animation.
	FBIKJointSettings make_limited_joint(float limit_deg, float stiffness = 0.0f)
	{
		FBIKJointSettings joint;
		joint.angular_stiffness = vec3(stiffness);
		for (IKLimitType& type : joint.angular_limit_types)
		{
			type = IKLimitType::LIMITED;
		}
		joint.angular_limits_deg = vec3(limit_deg);
		return joint;
	}

	std::vector<IKEffector> make_hand_effectors()
	{
		std::vector<IKEffector> effectors(2);
		effectors[0].joint_id = L_HAND;
		effectors[1].joint_id = R_HAND;
		for (IKEffector& effector : effectors)
		{
			effector.pole_vector_local = -vec3::forward;
			effector.pole_weight = 0.5f;
		}
		return effectors;
	}

	std::vector<FBIKJointSettings> make_joint_settings(float root_rotation_mobility, float root_position_mobility)
	{
		std::vector<FBIKJointSettings> joint_settings(JOINTS_COUNT);
		joint_settings[HIPS].angular_stiffness = vec3(1.0f - root_rotation_mobility);
		joint_settings[HIPS].linear_stiffness = vec3(1.0f - root_position_mobility);

		for (uint32 joint : { SPINE, SPINE1, SPINE2 })
		{
			joint_settings[joint] = make_limited_joint(30.0f, 0.5f);
		}

		// Without limits a clavicle is a free ball joint: a hand far to the other side turns it into the body.
		for (uint32 joint : { L_CLAVICLE, R_CLAVICLE })
		{
			joint_settings[joint] = make_limited_joint(30.0f);
		}
		return joint_settings;
	}

	ScenarioResult run_scenario(const TestRig& rig, const Scenario& scenario, FBIKSolverType solver_type)
	{
		std::vector<IKEffector> effectors = make_hand_effectors();
		const std::vector<FBIKJointSettings> joint_settings = make_joint_settings(scenario.root_rotation_mobility, scenario.root_position_mobility);

		const ref<FBIKSolver> solver = create_full_body_ik_solver(solver_type);
		EXPECT_TRUE(solver->init(&rig.skeleton, HIPS, effectors, joint_settings));

		FBIKSettings settings;

		const float dt = 1.0f / 60.0f;
		const uint32 frames = 360;

		std::vector<quat> previous_solved(JOINTS_COUNT);
		std::vector<quat> previous_animation(JOINTS_COUNT);
		std::vector<quat> previous_solved_delta(JOINTS_COUNT);
		std::vector<quat> previous_animation_delta(JOINTS_COUNT);

		float worst_jump = 0.0f;
		uint32 worst_frame = 0;
		uint32 worst_joint = 0;
		float max_error = 0.0f;
		uint32 max_iterations = 0;

		ScenarioResult result;

		float worst_twist = 0.0f;
		uint32 worst_twist_frame = 0;
		uint32 worst_twist_joint = 0;

		for (uint32 frame = 0; frame < frames; ++frame)
		{
			const float time = frame * dt;

			const trs world_transform = trs(vec3(0.0f, 0.0f, -1.0f + scenario.walk_speed * time), quat::identity, vec3(1.0f));

			const quat circle_rotation = rotate_from_to(vec3(0.0f, 0.0f, 1.0f), scenario.target_circle_axis);
			const vec3 circle = circle_rotation * vec3(std::cos(time), std::sin(time), 0.0f) * scenario.target_circle_radius;
			effectors[0].target_position = scenario.left_target + circle;
			effectors[1].target_position = scenario.right_target - circle;

			SkeletonPose animation_pose = rig.animate(time);
			SkeletonPose pose = animation_pose;
			solver->solve(pose, world_transform, effectors, settings);

			max_error = std::max(max_error, solver->get_last_error());
			max_iterations = std::max(max_iterations, solver->get_last_iterations());

			for (uint32 joint = 0; joint < JOINTS_COUNT; ++joint)
			{
				const quat solved = pose.get_joint_rotation(joint);
				const quat animated = animation_pose.get_joint_rotation(joint);

				// The decomposition is singular for a bone opposite to its rest direction.
				const vec3 axis = rig.bone_children[joint] != INVALID_JOINT ? normalize(rig.offsets[rig.bone_children[joint]]) : vec3(0.0f);
				if (rig.bone_children[joint] != INVALID_JOINT && dot(solved * axis, axis) > -0.7f && dot(animated * axis, axis) > -0.7f)
				{
					const float twist = std::abs(twist_from_animation(solved, animated, axis));
					if (twist > worst_twist)
					{
						worst_twist = twist;
						worst_twist_frame = frame;
						worst_twist_joint = joint;
					}
				}
				if (joint == L_CLAVICLE || joint == R_CLAVICLE)
				{
					const vec3 axis = normalize(rig.offsets[rig.bone_children[joint]]);
					result.worst_clavicle_swing_deg = std::max(result.worst_clavicle_swing_deg, rad2deg(std::acos(clamp(dot(solved * axis, animated * axis), -1.0f, 1.0f))));
				}

				if (frame > 0)
				{
					const quat solved_delta = conjugate(previous_solved[joint]) * solved;
					const quat animation_delta = conjugate(previous_animation[joint]) * animated;
					if (frame > 1)
					{
						const float jump = rotation_angle(previous_solved_delta[joint], solved_delta) - rotation_angle(previous_animation_delta[joint], animation_delta);
						if (jump > worst_jump)
						{
							worst_jump = jump;
							worst_frame = frame;
							worst_joint = joint;
						}
					}
					previous_solved_delta[joint] = solved_delta;
					previous_animation_delta[joint] = animation_delta;
				}
				previous_solved[joint] = solved;
				previous_animation[joint] = animated;
			}
		}

		printf("[FBIK %s] %-28s worst jerk %7.3f deg (frame %3u, joint %2u), worst twist %7.3f deg (frame %3u, joint %2u), clavicle swing %6.2f deg, max iterations %2u, max error %.4f\n",
			solver_type == FBIKSolverType::FABRIK ? "fabrik" : "jacobian", scenario.name, rad2deg(worst_jump), worst_frame, worst_joint, rad2deg(worst_twist), worst_twist_frame, worst_twist_joint, result.worst_clavicle_swing_deg, max_iterations, max_error);

		result.worst_jerk_deg = rad2deg(worst_jump);
		result.worst_twist_deg = rad2deg(worst_twist);
		result.max_error = max_error;
		return result;
	}
}

TEST(IK, FullBodyIKTemporalStability)
{
	const TestRig rig;

	Scenario scenarios[11];

	scenarios[0].name = "reachable";
	scenarios[0].left_target = vec3(0.3f, 1.2f, -0.6f);
	scenarios[0].right_target = vec3(-0.3f, 1.2f, -0.6f);

	scenarios[1].name = "out of reach";
	scenarios[1].left_target = vec3(0.4f, 1.5f, -0.1f);
	scenarios[1].right_target = vec3(-0.5f, 1.3f, -0.2f);

	scenarios[2].name = "walk through targets";
	scenarios[2].walk_speed = 0.35f;
	scenarios[2].left_target = vec3(0.3f, 1.3f, -0.2f);
	scenarios[2].right_target = vec3(-0.3f, 1.3f, -0.2f);

	scenarios[3].name = "walk, mobile root";
	scenarios[3].walk_speed = 0.35f;
	scenarios[3].root_rotation_mobility = 0.5f;
	scenarios[3].root_position_mobility = 0.5f;
	scenarios[3].left_target = vec3(0.3f, 1.3f, -0.2f);
	scenarios[3].right_target = vec3(-0.3f, 1.3f, -0.2f);

	scenarios[4].name = "moving targets";
	scenarios[4].target_circle_radius = 0.4f;
	scenarios[4].left_target = vec3(0.3f, 1.3f, -0.3f);
	scenarios[4].right_target = vec3(-0.3f, 1.3f, -0.3f);

	scenarios[5].name = "overhead";
	scenarios[5].target_circle_radius = 0.3f;
	scenarios[5].target_circle_axis = vec3(0.0f, 1.0f, 0.0f);
	scenarios[5].left_target = vec3(0.3f, 1.9f, -1.0f);
	scenarios[5].right_target = vec3(-0.3f, 1.9f, -1.0f);

	scenarios[6].name = "behind";
	scenarios[6].target_circle_radius = 0.3f;
	scenarios[6].target_circle_axis = vec3(1.0f, 0.0f, 0.0f);
	scenarios[6].left_target = vec3(0.3f, 1.2f, -1.45f);
	scenarios[6].right_target = vec3(-0.3f, 1.2f, -1.45f);

	// A hand moving near its shoulder (about (+-0.18, 1.46, -1.0)). A target through the limb root necessarily flips the limb.
	scenarios[7].name = "near limb root";
	scenarios[7].target_circle_radius = 0.03f;
	scenarios[7].left_target = vec3(0.2f, 1.5f, -0.88f);
	scenarios[7].right_target = vec3(-0.2f, 1.5f, -0.88f);

	scenarios[8].name = "static near limb root";
	scenarios[8].left_target = vec3(0.2f, 1.42f, -0.92f);
	scenarios[8].right_target = vec3(-0.2f, 1.42f, -0.92f);

	// Walks to the targets until they are 0.1 in front of the shoulders.
	scenarios[9].name = "approach limb root";
	scenarios[9].walk_speed = 0.15f;
	scenarios[9].left_target = vec3(0.2f, 1.42f, 0.0f);
	scenarios[9].right_target = vec3(-0.2f, 1.42f, 0.0f);

	// The right hand reaches far to the left.
	scenarios[10].name = "across the body";
	scenarios[10].target_circle_radius = 0.1f;
	scenarios[10].left_target = vec3(0.3f, 1.2f, -0.6f);
	scenarios[10].right_target = vec3(0.5f, 1.35f, -0.75f);

	for (FBIKSolverType solver_type : { FBIKSolverType::FABRIK, FBIKSolverType::JACOBIAN })
	{
		for (const Scenario& scenario : scenarios)
		{
			const ScenarioResult result = run_scenario(rig, scenario, solver_type);
			EXPECT_LT(result.worst_jerk_deg, 8.0f) << scenario.name;
			EXPECT_LT(result.worst_twist_deg, 90.0f) << scenario.name;
			EXPECT_LT(result.worst_clavicle_swing_deg, 31.0f) << scenario.name;
		}
	}
}

// A target straight in front of the right shoulder: the limb bends before the body moves and is straight when the body helps.
TEST(IK, FullBodyIKLimbBeforeBody)
{
	const TestRig rig;
	const trs world_transform = trs(vec3(0.0f, 0.0f, -1.0f), quat::identity, vec3(1.0f));
	const vec3 shoulder = vec3(-0.18f, 1.46f, -1.0f);

	for (FBIKSolverType solver_type : { FBIKSolverType::FABRIK, FBIKSolverType::JACOBIAN })
	{
		std::vector<IKEffector> effectors = make_hand_effectors();
		effectors[0].enabled = false;

		const ref<FBIKSolver> solver = create_full_body_ik_solver(solver_type);
		ASSERT_TRUE(solver->init(&rig.skeleton, HIPS, effectors, make_joint_settings(0.0f, 0.0f)));

		const FBIKSettings settings;

		for (float distance : { 0.2f, 0.3f, 0.4f, 0.5f, 0.54f, 0.58f, 0.62f, 0.7f, 0.8f })
		{
			effectors[1].target_position = shoulder + vec3(0.0f, 0.0f, distance);

			const SkeletonPose animation_pose = rig.animate(0.0f);
			SkeletonPose pose = animation_pose;
			ASSERT_TRUE(solver->solve(pose, world_transform, effectors, settings));

			float body_angle = 0.0f;
			for (uint32 joint : { SPINE, SPINE1, SPINE2, R_CLAVICLE })
			{
				body_angle = std::max(body_angle, rad2deg(rotation_angle(pose.get_joint_rotation(joint), animation_pose.get_joint_rotation(joint))));
			}

			vec3 positions[JOINTS_COUNT];
			for (size_t n = 0; n < solver->get_nodes().size(); ++n)
			{
				positions[solver->get_nodes()[n].joint_id] = solver->get_solved_positions()[n];
			}
			const vec3 upper_bone = normalize(positions[R_FOREARM] - positions[R_ARM]);
			const vec3 lower_bone = normalize(positions[R_HAND] - positions[R_FOREARM]);
			const float elbow_bend = rad2deg(std::acos(clamp(dot(upper_bone, lower_bone), -1.0f, 1.0f)));

			if (distance <= 0.5f)
			{
				EXPECT_LT(body_angle, 0.1f) << distance;
				EXPECT_LT(solver->get_last_error(), 1e-3f) << distance;
			}
			else
			{
				EXPECT_LT(elbow_bend, 1.0f) << distance;
				if (distance <= 0.7f && solver_type == FBIKSolverType::JACOBIAN)
				{
					EXPECT_LT(solver->get_last_error(), 0.003f) << distance;
				}
			}
		}
	}
}
