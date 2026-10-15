#include <gtest/gtest.h>

#include <motion_matching/motion/spring_motion_utils.h>
#include <motion_matching/motion/motion_utils.h>
#include <motion_matching/common.h>

#include <core/math.h>

namespace
{
	using namespace era_engine;

	constexpr float dt = 1.0f / 60.0f;
}

TEST(MotionMatching_Springs, DamperHalvesDistanceAfterHalflife)
{
	EXPECT_NEAR(SpringMotionUtils::damper_exact(0.0f, 10.0f, 0.5f, 0.5f), 5.0f, 0.1f);
	EXPECT_FLOAT_EQ(SpringMotionUtils::damper_exact(3.0f, 10.0f, 0.5f, 0.0f), 3.0f);
	EXPECT_NEAR(SpringMotionUtils::damper_exact(0.0f, 10.0f, 0.5f, 100.0f), 10.0f, 1e-3f);

	const vec3 result = SpringMotionUtils::damper_exact(vec3(0.0f), vec3(2.0f, -4.0f, 6.0f), 0.2f, 0.2f);
	EXPECT_TRUE(fuzzy_equals(result, vec3(1.0f, -2.0f, 3.0f), 0.05f));

	EXPECT_NEAR(SpringMotionUtils::damp_adjustment_exact(10.0f, 0.5f, 0.5f), 5.0f, 0.1f);

	const quat goal(vec3(0.0f, 1.0f, 0.0f), 1.0f);
	EXPECT_TRUE(fuzzy_equals(SpringMotionUtils::damper_exact(quat::identity, goal, 0.1f, 100.0f), goal, 1e-3f));
	EXPECT_TRUE(fuzzy_equals(SpringMotionUtils::damper_exact(quat::identity, goal, 0.1f, 0.0f), quat::identity, 1e-3f));
}

TEST(MotionMatching_Springs, SpringDamperConvergesToGoal)
{
	float x = 0.0f;
	float v = 0.0f;
	float previous_x = x;
	bool monotonic = true;

	for (int32 i = 0; i < 300; ++i)
	{
		SpringMotionUtils::simple_spring_damper_exact(x, v, 1.0f, 0.2f, dt);
		monotonic &= x >= previous_x - 1e-6f;
		previous_x = x;
	}

	EXPECT_TRUE(monotonic);
	EXPECT_NEAR(x, 1.0f, 1e-3f);
	EXPECT_NEAR(v, 0.0f, 1e-3f);

	vec3 position(0.0f);
	vec3 velocity(0.0f);
	for (int32 i = 0; i < 300; ++i)
	{
		SpringMotionUtils::simple_spring_damper_exact(position, velocity, vec3(1.0f, 2.0f, 3.0f), 0.2f, dt);
	}
	EXPECT_TRUE(fuzzy_equals(position, vec3(1.0f, 2.0f, 3.0f), 1e-3f));

	quat rotation = quat::identity;
	vec3 angular_velocity(0.0f);
	const quat target(normalize(vec3(1.0f, 1.0f, 0.0f)), 1.2f);
	for (int32 i = 0; i < 300; ++i)
	{
		SpringMotionUtils::simple_spring_damper_exact(rotation, angular_velocity, target, 0.2f, dt);
	}
	EXPECT_TRUE(fuzzy_equals(rotation, target, 1e-3f)) << rotation << " vs " << target;
	EXPECT_NEAR(length(angular_velocity), 0.0f, 1e-3f) << angular_velocity;
}

TEST(MotionMatching_Springs, DecaySpringDamperGoesToZero)
{
	float x = 2.0f;
	float v = 1.0f;
	vec3 x3(1.0f, -1.0f, 2.0f);
	vec3 v3(0.5f);
	quat q(vec3(0.0f, 0.0f, 1.0f), 0.8f);
	vec3 w(0.0f, 0.3f, 0.0f);

	for (int32 i = 0; i < 300; ++i)
	{
		SpringMotionUtils::decay_spring_damper_exact(x, v, 0.2f, dt);
		SpringMotionUtils::decay_spring_damper_exact(x3, v3, 0.2f, dt);
		SpringMotionUtils::decay_spring_damper_exact(q, w, 0.2f, dt);
	}

	EXPECT_NEAR(x, 0.0f, 1e-3f);
	EXPECT_NEAR(v, 0.0f, 1e-3f);
	EXPECT_TRUE(fuzzy_equals(x3, vec3(0.0f), 1e-3f));
	EXPECT_TRUE(fuzzy_equals(q, quat::identity, 1e-3f));
}

TEST(MotionMatching_Springs, InertializationIsContinuous)
{
	const vec3 source_position(1.0f, 0.0f, 0.0f);
	const vec3 source_velocity(0.0f, 1.0f, 0.0f);
	const vec3 destination_position(3.0f, 0.0f, 1.0f);
	const vec3 destination_velocity(0.0f, 0.0f, 2.0f);

	vec3 offset_position(0.0f);
	vec3 offset_velocity(0.0f);
	SpringMotionUtils::inertialize_transition(offset_position, offset_velocity, source_position, source_velocity, destination_position, destination_velocity);

	vec3 out_position, out_velocity;
	SpringMotionUtils::inertialize_update(out_position, out_velocity, offset_position, offset_velocity, destination_position, destination_velocity, 0.1f, 0.0f);

	EXPECT_TRUE(fuzzy_equals(out_position, source_position, 1e-4f));
	EXPECT_TRUE(fuzzy_equals(out_velocity, source_velocity, 1e-4f));

	for (int32 i = 0; i < 300; ++i)
	{
		SpringMotionUtils::inertialize_update(out_position, out_velocity, offset_position, offset_velocity, destination_position, destination_velocity, 0.1f, dt);
	}

	EXPECT_TRUE(fuzzy_equals(out_position, destination_position, 1e-3f));
	EXPECT_TRUE(fuzzy_equals(out_velocity, destination_velocity, 1e-3f));

	const quat source_rotation(vec3(0.0f, 1.0f, 0.0f), 0.5f);
	const quat destination_rotation(vec3(0.0f, 1.0f, 0.0f), -0.7f);

	quat offset_rotation = quat::identity;
	vec3 offset_angular_velocity(0.0f);
	SpringMotionUtils::inertialize_transition(offset_rotation, offset_angular_velocity, source_rotation, vec3(0.0f), destination_rotation, vec3(0.0f));

	quat out_rotation;
	vec3 out_angular_velocity;
	SpringMotionUtils::inertialize_update(out_rotation, out_angular_velocity, offset_rotation, offset_angular_velocity, destination_rotation, vec3(0.0f), 0.1f, 0.0f);
	EXPECT_TRUE(fuzzy_equals(out_rotation, source_rotation, 1e-4f));

	for (int32 i = 0; i < 300; ++i)
	{
		SpringMotionUtils::inertialize_update(out_rotation, out_angular_velocity, offset_rotation, offset_angular_velocity, destination_rotation, vec3(0.0f), 0.1f, dt);
	}
	EXPECT_TRUE(fuzzy_equals(out_rotation, destination_rotation, 1e-3f));
}

TEST(MotionMatching_MotionUtils, DesiredVelocityScalesInput)
{
	const float forward_speed = 4.0f;
	const float side_speed = 3.0f;
	const float back_speed = 2.0f;

	EXPECT_TRUE(fuzzy_equals(MotionUtils::desired_velocity_update(vec3(0.0f, 0.0f, -1.0f), quat::identity, forward_speed, side_speed, back_speed), vec3(0.0f, 0.0f, -4.0f)));
	EXPECT_TRUE(fuzzy_equals(MotionUtils::desired_velocity_update(vec3(0.0f, 0.0f, 1.0f), quat::identity, forward_speed, side_speed, back_speed), vec3(0.0f, 0.0f, 2.0f)));
	EXPECT_TRUE(fuzzy_equals(MotionUtils::desired_velocity_update(vec3(1.0f, 0.0f, 0.0f), quat::identity, forward_speed, side_speed, back_speed), vec3(3.0f, 0.0f, 0.0f)));

	const quat yaw(vec3(0.0f, 1.0f, 0.0f), M_PI_OVER_2);
	EXPECT_TRUE(fuzzy_equals(MotionUtils::desired_velocity_update(vec3(1.0f, 0.0f, 0.0f), yaw, forward_speed, side_speed, back_speed), vec3(0.0f, 0.0f, -3.0f)));
	EXPECT_TRUE(fuzzy_equals(MotionUtils::desired_velocity_update(vec3(0.0f), yaw, forward_speed, side_speed, back_speed), vec3(0.0f)));
}

TEST(MotionMatching_MotionUtils, DesiredRotationFollowsVelocity)
{
	const quat current(vec3(0.0f, 1.0f, 0.0f), 0.3f);

	EXPECT_EQ(MotionUtils::desired_rotation_update(current, false, 0.0f, false, vec3(1.0f, 0.0f, 0.0f)), current);

	const vec3 velocity(1.0f, 0.0f, 1.0f);
	const quat facing = MotionUtils::desired_rotation_update(current, true, 0.0f, false, velocity);
	EXPECT_TRUE(fuzzy_equals(facing * vec3(0.0f, 0.0f, 1.0f), normalize(velocity), 1e-4f));

	const quat strafing = MotionUtils::desired_rotation_update(current, true, 0.0f, true, velocity);
	EXPECT_TRUE(fuzzy_equals(strafing * vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, -1.0f), 1e-4f));
}

TEST(MotionMatching_MotionUtils, SimulationReachesDesiredVelocity)
{
	vec3 position(0.0f);
	vec3 velocity(0.0f);
	vec3 acceleration(0.0f);
	const vec3 desired_velocity(2.0f, 0.0f, -1.0f);

	vec3 previous_position = position;
	for (int32 i = 0; i < 600; ++i)
	{
		previous_position = position;
		MotionUtils::simulation_positions_update(position, velocity, acceleration, desired_velocity, 0.27f, dt);
	}

	EXPECT_TRUE(fuzzy_equals(velocity, desired_velocity, 1e-3f));
	EXPECT_TRUE(fuzzy_equals(acceleration, vec3(0.0f), 1e-3f));
	EXPECT_TRUE(fuzzy_equals((position - previous_position) / dt, desired_velocity, 1e-2f));
}

TEST(MotionMatching_MotionUtils, SimulationKeepsConstantVelocity)
{
	vec3 position(1.0f, 0.0f, 1.0f);
	vec3 velocity(0.0f, 0.0f, 3.0f);
	vec3 acceleration(0.0f);

	MotionUtils::simulation_positions_update(position, velocity, acceleration, velocity, 0.27f, 0.5f);

	EXPECT_TRUE(fuzzy_equals(position, vec3(1.0f, 0.0f, 2.5f), 1e-4f));
	EXPECT_TRUE(fuzzy_equals(velocity, vec3(0.0f, 0.0f, 3.0f), 1e-4f));
}

TEST(MotionMatching_MotionUtils, SimulationRotationReachesTarget)
{
	quat rotation = quat::identity;
	vec3 angular_velocity(0.0f);
	const quat desired(vec3(0.0f, 1.0f, 0.0f), 2.0f);

	for (int32 i = 0; i < 600; ++i)
	{
		MotionUtils::simulation_rotations_update(rotation, angular_velocity, desired, 0.27f, dt);
	}

	EXPECT_TRUE(fuzzy_equals(rotation, desired, 1e-3f)) << rotation << " vs " << desired << ", angular velocity " << angular_velocity;

	float gait = 1.0f;
	float gait_velocity = 0.0f;
	for (int32 i = 0; i < 120; ++i)
	{
		MotionUtils::desired_gait_update(gait, gait_velocity, dt);
	}
	EXPECT_NEAR(gait, 0.0f, 1e-3f);
}
