#include <gtest/gtest.h>

#include <core/math.h>
#include <core/hash.h>
#include <core/random.h>
#include <core/bounding_volumes.h>

#include <utility>

namespace
{
	using namespace era_engine;

	constexpr float tolerance = 1e-4f;

	const vec3 test_axes[] = {
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f),
		normalize(vec3(1.0f, 2.0f, -3.0f)),
		normalize(vec3(-0.3f, 0.5f, 0.8f))
	};

	const float test_angles[] = { 0.1f, 0.5f, 1.0f, 2.0f, 3.0f };
}

TEST(Core_Math, ScalarHelpers)
{
	EXPECT_NEAR(deg2rad(180.0f), M_PI, tolerance);
	EXPECT_NEAR(rad2deg(M_PI_OVER_2), 90.0f, tolerance);
	EXPECT_NEAR(rad2deg(deg2rad(37.0f)), 37.0f, tolerance);

	EXPECT_FLOAT_EQ(lerp(2.0f, 4.0f, 0.5f), 3.0f);
	EXPECT_FLOAT_EQ(inverse_lerp(2.0f, 4.0f, 3.0f), 0.5f);
	EXPECT_FLOAT_EQ(remap(5.0f, 0.0f, 10.0f, 100.0f, 200.0f), 150.0f);

	EXPECT_FLOAT_EQ(clamp(5.0f, 0.0f, 1.0f), 1.0f);
	EXPECT_FLOAT_EQ(clamp(-5.0f, 0.0f, 1.0f), 0.0f);
	EXPECT_FLOAT_EQ(clamp(0.3f, 0.0f, 1.0f), 0.3f);
	EXPECT_EQ(clamp(10, -2, 3), 3);
	EXPECT_EQ(clamp(7u, 8u, 9u), 8u);
	EXPECT_FLOAT_EQ(clamp01(1.5f), 1.0f);
	EXPECT_FLOAT_EQ(saturate(-0.5f), 0.0f);

	EXPECT_FLOAT_EQ(smoothstep(0.0f), 0.0f);
	EXPECT_FLOAT_EQ(smoothstep(0.5f), 0.5f);
	EXPECT_FLOAT_EQ(smoothstep(1.0f), 1.0f);
	EXPECT_FLOAT_EQ(smoothstep(0.0f, 10.0f, 20.0f), 1.0f);

	EXPECT_EQ(bucketize(10u, 4u), 3u);
	EXPECT_EQ(bucketize(8u, 4u), 2u);
	EXPECT_EQ(bucketize(uint64(9), uint64(3)), uint64(3));

	EXPECT_NEAR(frac(2.75f), 0.75f, tolerance);
}

TEST(Core_Math, BitHelpers)
{
	EXPECT_EQ(index_of_least_significant_set_bit(0b1000u), 3u);
	EXPECT_EQ(index_of_least_significant_set_bit(0b1010u), 1u);
	EXPECT_EQ(index_of_least_significant_set_bit(0u), uint32(-1));
	EXPECT_EQ(index_of_most_significant_set_bit(0b1010u), 3u);
	EXPECT_EQ(index_of_most_significant_set_bit(1u), 0u);
	EXPECT_EQ(index_of_most_significant_set_bit(0u), uint32(-1));

	EXPECT_EQ(log2(1u), 0u);
	EXPECT_EQ(log2(8u), 3u);
	EXPECT_EQ(log2(9u), 4u);
	EXPECT_EQ(log2(int32(16)), 4);

	EXPECT_TRUE(is_power_of_two(1u));
	EXPECT_TRUE(is_power_of_two(64u));
	EXPECT_FALSE(is_power_of_two(48u));

	EXPECT_EQ(align_to_power_of_two(0u), 0u);
	EXPECT_EQ(align_to_power_of_two(33u), 64u);
	EXPECT_EQ(align_to_power_of_two(64u), 64u);
}

TEST(Core_Math, EasingEndpoints)
{
	using EasingFunction = float(*)(float);

	const std::pair<const char*, EasingFunction> functions[] = {
		{ "ease_in_quadratic", ease_in_quadratic }, { "ease_out_quadratic", ease_out_quadratic }, { "ease_in_out_quadratic", ease_in_out_quadratic },
		{ "ease_in_cubic", ease_in_cubic }, { "ease_out_cubic", ease_out_cubic }, { "ease_in_out_cubic", ease_in_out_cubic },
		{ "ease_in_quartic", ease_in_quartic }, { "ease_out_quartic", ease_out_quartic }, { "ease_in_out_quartic", ease_in_out_quartic },
		{ "ease_in_quintic", ease_in_quintic }, { "ease_out_quintic", ease_out_quintic }, { "ease_in_out_quintic", ease_in_out_quintic },
		{ "ease_in_sine", ease_in_sine }, { "ease_out_sine", ease_out_sine }, { "ease_in_out_sine", ease_in_out_sine },
		{ "ease_in_circular", ease_in_circular }, { "ease_out_circular", ease_out_circular }, { "ease_in_out_circular", ease_in_out_circular },
		{ "ease_in_exponential", ease_in_exponential }, { "ease_out_exponential", ease_out_exponential }, { "ease_in_out_exponential", ease_in_out_exponential },
		{ "in_back", in_back }, { "out_back", out_back }, { "in_out_back", in_out_back },
		{ "in_bounce", in_bounce }, { "out_bounce", out_bounce }, { "in_out_bounce", in_out_bounce }
	};

	for (const auto& [name, function] : functions)
	{
		SCOPED_TRACE(name);
		EXPECT_NEAR(function(0.0f), 0.0f, 1e-3f);
		EXPECT_NEAR(function(1.0f), 1.0f, 1e-3f);
	}
}

TEST(Core_Math, AngleWrapping)
{
	EXPECT_NEAR(angle_to_zero_to_two_pi(-M_PI_OVER_2), 1.5f * M_PI, tolerance);
	EXPECT_NEAR(angle_to_zero_to_two_pi(M_TAU + 1.0f), 1.0f, tolerance);
	EXPECT_NEAR(angle_to_zero_to_two_pi(0.5f), 0.5f, tolerance);

	EXPECT_NEAR(angle_to_neg_pi_to_pi(1.5f * M_PI), -M_PI_OVER_2, tolerance);
	EXPECT_NEAR(angle_to_neg_pi_to_pi(-1.5f * M_PI), M_PI_OVER_2, tolerance);
	EXPECT_NEAR(angle_to_neg_pi_to_pi(0.25f), 0.25f, tolerance);

	EXPECT_NEAR(map_value(5.0f, 0.0f, 10.0f, -1.0f, 1.0f), 0.0f, tolerance);
}

TEST(Core_Math, VectorOperations)
{
	const vec3 a(1.0f, 2.0f, 3.0f);
	const vec3 b(4.0f, -5.0f, 6.0f);

	EXPECT_EQ(a + b, vec3(5.0f, -3.0f, 9.0f));
	EXPECT_EQ(a - b, vec3(-3.0f, 7.0f, -3.0f));
	EXPECT_EQ(a * b, vec3(4.0f, -10.0f, 18.0f));
	EXPECT_EQ(a * 2.0f, vec3(2.0f, 4.0f, 6.0f));
	EXPECT_EQ(2.0f * a, a * 2.0f);
	EXPECT_EQ(-a, vec3(-1.0f, -2.0f, -3.0f));
	EXPECT_TRUE(fuzzy_equals(b / 2.0f, vec3(2.0f, -2.5f, 3.0f)));

	vec3 c = a;
	c += b;
	c -= b;
	c *= 3.0f;
	c /= 3.0f;
	EXPECT_TRUE(fuzzy_equals(c, a));

	EXPECT_FLOAT_EQ(dot(a, b), 12.0f);
	EXPECT_FLOAT_EQ(squared_length(a), 14.0f);
	EXPECT_FLOAT_EQ(length(vec3(3.0f, 4.0f, 0.0f)), 5.0f);
	EXPECT_FLOAT_EQ(distance(vec3(1.0f, 1.0f, 1.0f), vec3(1.0f, 4.0f, 5.0f)), 5.0f);
	EXPECT_NEAR(length(normalize(b)), 1.0f, tolerance);
	EXPECT_EQ(noz(vec3(0.0f)), vec3(0.0f));
	EXPECT_NEAR(length(noz(a)), 1.0f, tolerance);

	EXPECT_EQ(min(a, b), vec3(1.0f, -5.0f, 3.0f));
	EXPECT_EQ(max(a, b), vec3(4.0f, 2.0f, 6.0f));
	EXPECT_EQ(abs(b), vec3(4.0f, 5.0f, 6.0f));
	EXPECT_FLOAT_EQ(max_element(b), 6.0f);
	EXPECT_EQ(max_element_index(vec3(1.0f, 7.0f, 3.0f)), 1u);
	EXPECT_TRUE(fuzzy_equals(lerp(a, b, 0.5f), vec3(2.5f, -1.5f, 4.5f)));

	const vec4 v4(1.0f, 2.0f, 3.0f, 4.0f);
	EXPECT_FLOAT_EQ(dot(v4, v4), 30.0f);
	EXPECT_EQ(v4 + v4, v4 * 2.0f);

	const vec2 v2(3.0f, 4.0f);
	EXPECT_FLOAT_EQ(length(v2), 5.0f);
	EXPECT_FLOAT_EQ(cross(vec2(1.0f, 0.0f), vec2(0.0f, 1.0f)), 1.0f);
}

TEST(Core_Math, CrossProductIsRightHanded)
{
	EXPECT_EQ(cross(vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)), vec3(0.0f, 0.0f, 1.0f));
	EXPECT_EQ(cross(vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)), vec3(1.0f, 0.0f, 0.0f));
	EXPECT_EQ(cross(vec3::right, vec3::up), vec3::forward);

	const vec3 a(1.0f, 2.0f, 3.0f);
	const vec3 b(-2.0f, 0.5f, 4.0f);
	const vec3 c = cross(a, b);

	EXPECT_NEAR(dot(c, a), 0.0f, tolerance);
	EXPECT_NEAR(dot(c, b), 0.0f, tolerance);
	EXPECT_TRUE(fuzzy_equals(cross(b, a), -c));
}

TEST(Core_Math, VectorHelpers)
{
	const vec3 axis(0.0f, 1.0f, 0.0f);
	EXPECT_TRUE(fuzzy_equals(reject(vec3(1.0f, 2.0f, 3.0f), axis), vec3(1.0f, 0.0f, 3.0f)));

	EXPECT_NEAR(signed_angle(vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f), axis), M_PI_OVER_2, tolerance);
	EXPECT_NEAR(signed_angle(vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), axis), -M_PI_OVER_2, tolerance);

	for (const vec3& unit : test_axes)
	{
		const vec3 perpendicular = any_perpendicular(unit);
		EXPECT_NEAR(dot(perpendicular, unit), 0.0f, tolerance);
		EXPECT_GT(length(perpendicular), 0.1f);

		vec3 tangent, bitangent;
		get_tangents(unit, tangent, bitangent);
		EXPECT_NEAR(dot(tangent, unit), 0.0f, tolerance);
		EXPECT_NEAR(dot(bitangent, unit), 0.0f, tolerance);
		EXPECT_NEAR(dot(tangent, bitangent), 0.0f, tolerance);
	}

	float len = 0.0f;
	vec3 normal;
	decompose(vec3(0.0f, 0.0f, -4.0f), len, normal);
	EXPECT_FLOAT_EQ(len, 4.0f);
	EXPECT_TRUE(fuzzy_equals(normal, vec3(0.0f, 0.0f, -1.0f)));
}

TEST(Core_Math, QuaternionRotatesVectors)
{
	const quat yaw(vec3(0.0f, 1.0f, 0.0f), M_PI_OVER_2);

	EXPECT_TRUE(fuzzy_equals(yaw * vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 0.0f)));
	EXPECT_TRUE(fuzzy_equals(yaw * vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)));
	EXPECT_TRUE(fuzzy_equals(yaw * vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)));

	const quat pitch(vec3(1.0f, 0.0f, 0.0f), M_PI_OVER_2);
	EXPECT_TRUE(fuzzy_equals(pitch * vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)));

	EXPECT_TRUE(fuzzy_equals(quat::identity * vec3(1.0f, 2.0f, 3.0f), vec3(1.0f, 2.0f, 3.0f)));
}

TEST(Core_Math, QuaternionAlgebra)
{
	for (const vec3& axis : test_axes)
	{
		for (float angle : test_angles)
		{
			const quat q(axis, angle);
			const vec3 v(0.3f, -1.2f, 2.0f);

			EXPECT_NEAR(length(q.v4), 1.0f, tolerance);
			EXPECT_TRUE(fuzzy_equals(conjugate(q) * (q * v), v));
			EXPECT_TRUE(fuzzy_equals(q * conjugate(q), quat::identity));
			EXPECT_NEAR(length(q * v), length(v), tolerance);

			const quat twice = q * q;
			EXPECT_TRUE(fuzzy_equals(twice, quat(axis, angle * 2.0f)));
			EXPECT_NEAR(angle_between(quat::identity, q), angle, 1e-3f);

			vec3 out_axis;
			float out_angle = 0.0f;
			get_axis_rotation(q, out_axis, out_angle);
			EXPECT_NEAR(out_angle, angle, 1e-3f);
			EXPECT_TRUE(fuzzy_equals(out_axis, axis, 1e-3f));
		}
	}

	EXPECT_TRUE(fuzzy_equals(quat(0.0f, 0.0f, 0.0f, 1.0f), quat(-0.0f, -0.0f, -0.0f, -1.0f)));
	EXPECT_TRUE(fuzzy_equals(normalize(quat(0.0f, 0.0f, 0.0f, 3.0f)), quat::identity));
}

TEST(Core_Math, QuaternionInterpolation)
{
	const vec3 axis = normalize(vec3(1.0f, 1.0f, 0.0f));
	const quat from = quat::identity;
	const quat to(axis, 2.0f);

	EXPECT_TRUE(fuzzy_equals(slerp(from, to, 0.0f), from));
	EXPECT_TRUE(fuzzy_equals(slerp(from, to, 1.0f), to));
	EXPECT_TRUE(fuzzy_equals(slerp(from, to, 0.5f), quat(axis, 1.0f)));
	EXPECT_TRUE(fuzzy_equals(slerp_shortest(from, to, 0.25f), quat(axis, 0.5f)));

	EXPECT_TRUE(fuzzy_equals(nlerp(from, to, 0.0f), from));
	EXPECT_TRUE(fuzzy_equals(nlerp(from, to, 1.0f), to));
	EXPECT_NEAR(length(lerp(from, to, 0.3f).v4), 1.0f, tolerance);

	const quat flipped = quat(-to.x, -to.y, -to.z, -to.w);
	EXPECT_TRUE(fuzzy_equals(slerp_shortest(from, flipped, 0.5f), quat(axis, 1.0f)));
	EXPECT_TRUE(fuzzy_equals(nlerp_shortest(from, flipped, 1.0f), to));
}

TEST(Core_Math, QuaternionExpLog)
{
	for (const vec3& axis : test_axes)
	{
		for (float angle : test_angles)
		{
			const quat q(axis, angle);
			const vec3 scaled_axis = quat_to_scaled_angle_axis(q);

			EXPECT_TRUE(fuzzy_equals(scaled_axis, axis * angle, 1e-3f));
			EXPECT_TRUE(fuzzy_equals(quat_from_scaled_angle_axis(scaled_axis), q));
			EXPECT_TRUE(fuzzy_equals(exp(log(q), 1e-8f), q));
		}
	}

	const vec3 angular_velocity(0.0f, 2.0f, 0.0f);
	const float dt = 0.1f;
	const quat start(vec3(1.0f, 0.0f, 0.0f), 0.3f);
	const quat end = quat_integrate_angular_velocity(angular_velocity, start, dt);

	EXPECT_TRUE(fuzzy_equals(quat_differentiate_angular_velocity(end, start, dt), angular_velocity, 1e-3f));
}

TEST(Core_Math, QuaternionLogIsAccurateNearIdentity)
{
	const vec3 axis = normalize(vec3(0.3f, 1.0f, -0.2f));

	for (float angle : { 1e-5f, 1e-4f, 1e-3f, 1e-2f })
	{
		const vec3 scaled_axis = quat_to_scaled_angle_axis(quat(axis, angle));
		EXPECT_NEAR(length(scaled_axis), angle, angle * 1e-2f) << angle;
		EXPECT_TRUE(fuzzy_equals(normalize(scaled_axis), axis, 1e-3f)) << angle;
	}

	const quat slightly_scaled = quat(axis, 1e-3f) * (1.0f - 1e-6f);
	EXPECT_NEAR(length(quat_to_scaled_angle_axis(slightly_scaled)), 1e-3f, 1e-5f);
}

TEST(Core_Math, RotateFromTo)
{
	const vec3 pairs[][2] = {
		{ vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f) },
		{ vec3(0.0f, 0.0f, 1.0f), vec3(1.0f, 1.0f, 1.0f) },
		{ vec3(1.0f, 2.0f, 3.0f), vec3(-3.0f, 0.5f, 1.0f) },
		{ vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f) },
		{ vec3(1.0f, 0.0f, 0.0f), vec3(2.0f, 0.0f, 0.0f) }
	};

	for (const auto& pair : pairs)
	{
		const vec3 from = normalize(pair[0]);
		const vec3 to = normalize(pair[1]);

		EXPECT_TRUE(fuzzy_equals(rotate_from_to(pair[0], pair[1]) * from, to, 1e-3f));
		EXPECT_TRUE(fuzzy_equals(shortest_arc(from, to) * from, to, 1e-3f));
	}
}

TEST(Core_Math, TwistSwingDecomposition)
{
	const vec3 twist_axis(0.0f, 1.0f, 0.0f);
	const quat twist_part(twist_axis, 0.7f);
	const quat swing_part(vec3(1.0f, 0.0f, 0.0f), 0.4f);
	const quat q = swing_part * twist_part;

	quat twist, swing;
	decompose_quaternion_into_twist_and_swing(q, twist_axis, twist, swing);

	EXPECT_TRUE(fuzzy_equals(swing * twist, q));
	EXPECT_TRUE(fuzzy_equals(twist, twist_part));
	EXPECT_NEAR(dot(swing.v, twist_axis), 0.0f, tolerance);
	EXPECT_NEAR(get_twist_angle(q, twist_axis), 0.7f, 1e-3f);
	EXPECT_NEAR(get_twist_angle(quat(twist_axis, -0.5f), twist_axis), -0.5f, 1e-3f);
}

TEST(Core_Math, LookAtQuaternion)
{
	const vec3 forward = normalize(vec3(1.0f, 0.0f, 1.0f));
	const quat q = look_at_quaternion(forward, vec3(0.0f, 1.0f, 0.0f));

	EXPECT_NEAR(length(q.v4), 1.0f, tolerance);
	EXPECT_TRUE(fuzzy_equals(q * vec3(0.0f, 0.0f, -1.0f), forward, 1e-3f));
	EXPECT_NEAR(dot(q * vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)), 1.0f, 1e-3f);
}

TEST(Core_Math, QuaternionMatrixConversion)
{
	for (const vec3& axis : test_axes)
	{
		for (float angle : test_angles)
		{
			const quat q(axis, angle);
			const mat3 m = quaternion_to_mat3(q);
			const vec3 v(0.5f, -1.0f, 2.0f);

			EXPECT_TRUE(fuzzy_equals(m * v, q * v));
			EXPECT_NEAR(determinant(m), 1.0f, tolerance);
			EXPECT_TRUE(fuzzy_equals(m * transpose(m), mat3::identity));
			EXPECT_TRUE(fuzzy_equals(mat3_to_quaternion(m), q));
		}
	}
}

TEST(Core_Math, MatrixOperations)
{
	const mat3 m = quaternion_to_mat3(quat(normalize(vec3(1.0f, 2.0f, 3.0f)), 0.8f)) * 2.0f;

	EXPECT_TRUE(fuzzy_equals(m * mat3::identity, m));
	EXPECT_TRUE(fuzzy_equals(invert(m) * m, mat3::identity));
	EXPECT_NEAR(determinant(m), 8.0f, 1e-3f);
	EXPECT_TRUE(fuzzy_equals(transpose(transpose(m)), m));
	EXPECT_NEAR(trace(mat3::identity), 3.0f, tolerance);
	EXPECT_TRUE(fuzzy_equals(m + m, m * 2.0f));
	EXPECT_TRUE(fuzzy_equals(m - m, mat3::zero));

	const mat4 model = create_model_matrix(vec3(1.0f, 2.0f, 3.0f), quat(vec3(0.0f, 1.0f, 0.0f), 0.6f), vec3(2.0f, 1.0f, 0.5f));
	EXPECT_TRUE(fuzzy_equals(invert(model) * model, mat4::identity));
	EXPECT_TRUE(fuzzy_equals(invert_affine(model), invert(model)));
	EXPECT_NEAR(determinant(model), 1.0f, 1e-3f);
	EXPECT_TRUE(fuzzy_equals(transpose(transpose(model)), model));
	EXPECT_NEAR(trace(mat4::identity), 4.0f, tolerance);

	const mat4 translation = create_translation_matrix(vec3(1.0f, -2.0f, 3.0f));
	EXPECT_TRUE(fuzzy_equals(transform_position(translation, vec3(1.0f)), vec3(2.0f, -1.0f, 4.0f)));
	EXPECT_TRUE(fuzzy_equals(transform_direction(translation, vec3(1.0f)), vec3(1.0f)));

	const mat2 m2(2.0f, 1.0f, 1.0f, 3.0f);
	EXPECT_NEAR(determinant(m2), 5.0f, tolerance);
	EXPECT_TRUE(fuzzy_equals(invert(m2) * m2, mat2::identity));
}

TEST(Core_Math, OuterProductAndSkewMatrix)
{
	const vec3 a(1.0f, 2.0f, 3.0f);
	const vec3 b(-1.0f, 0.5f, 2.0f);
	const vec3 v(0.3f, -0.7f, 1.1f);

	EXPECT_TRUE(fuzzy_equals(outer_product(a, b) * v, a * dot(b, v)));
	EXPECT_TRUE(fuzzy_equals(get_skew_matrix(a) * v, cross(a, v)));
}

TEST(Core_Math, LinearSystems)
{
	const mat3 a(
		4.0f, 1.0f, 2.0f,
		1.0f, 5.0f, 3.0f,
		2.0f, 3.0f, 6.0f);
	const vec3 x(1.0f, -2.0f, 0.5f);

	EXPECT_TRUE(fuzzy_equals(solve_linear_system(a, a * x), x, 1e-3f));

	const mat2 a2(3.0f, 1.0f, 1.0f, 2.0f);
	const vec2 x2(2.0f, -1.0f);

	EXPECT_TRUE(fuzzy_equals(solve_linear_system(a2, a2 * x2), x2, 1e-3f));
}

TEST(Core_Math, TrsComposition)
{
	const trs a(vec3(1.0f, 2.0f, 3.0f), quat(vec3(0.0f, 1.0f, 0.0f), 0.5f), vec3(2.0f));
	const trs b(vec3(-1.0f, 0.5f, 4.0f), quat(vec3(1.0f, 0.0f, 0.0f), -0.3f), vec3(0.5f));
	const vec3 p(0.2f, -1.0f, 3.0f);

	EXPECT_TRUE(fuzzy_equals(trs::identity * a, a));
	EXPECT_TRUE(fuzzy_equals(a * trs::identity, a));
	EXPECT_TRUE(fuzzy_equals(transform_position(a * b, p), transform_position(a, transform_position(b, p)), 1e-3f));

	EXPECT_TRUE(fuzzy_equals(invert(a) * a, trs::identity));
	EXPECT_TRUE(fuzzy_equals(a * invert(a), trs::identity));
	EXPECT_TRUE(fuzzy_equals(inverse_transform_position(a, transform_position(a, p)), p, 1e-3f));
	EXPECT_TRUE(fuzzy_equals(inverse_transform_direction(a, transform_direction(a, p)), p, 1e-3f));
	EXPECT_TRUE(fuzzy_equals(transform_direction(trs(vec3(5.0f)), p), p));

	EXPECT_TRUE(fuzzy_equals(lerp(a, b, 0.0f), a));
	EXPECT_TRUE(fuzzy_equals(lerp(a, b, 1.0f), b));
}

TEST(Core_Math, TrsMatrixConversion)
{
	const trs t(vec3(1.0f, -2.0f, 3.0f), quat(normalize(vec3(1.0f, 1.0f, 0.0f)), 0.9f), vec3(1.5f, 2.0f, 0.5f));
	const mat4 m = trs_to_mat4(t);
	const vec3 p(0.5f, 0.25f, -1.0f);

	EXPECT_TRUE(fuzzy_equals(transform_position(m, p), transform_position(t, p), 1e-3f));
	EXPECT_TRUE(fuzzy_equals(mat4_to_trs(m), t, 1e-3f));
}

TEST(Core_Math, Barycentrics)
{
	const vec3 a(0.0f, 0.0f, 0.0f);
	const vec3 b(1.0f, 0.0f, 0.0f);
	const vec3 c(0.0f, 1.0f, 0.0f);

	const vec3 at_a = get_barycentric_coordinates(a, b, c, a);
	EXPECT_TRUE(fuzzy_equals(at_a, vec3(1.0f, 0.0f, 0.0f)));

	const vec3 center = get_barycentric_coordinates(a, b, c, (a + b + c) / 3.0f);
	EXPECT_TRUE(fuzzy_equals(center, vec3(1.0f / 3.0f)));
	EXPECT_TRUE(inside_triangle(center));

	const vec3 outside = get_barycentric_coordinates(a, b, c, vec3(1.0f, 1.0f, 0.0f));
	EXPECT_FALSE(inside_triangle(outside));
	EXPECT_NEAR(outside.x + outside.y + outside.z, 1.0f, tolerance);

	const vec3 bary2d = get_barycentric_coordinates(vec2(0.0f, 0.0f), vec2(2.0f, 0.0f), vec2(0.0f, 2.0f), vec2(1.0f, 0.0f));
	EXPECT_TRUE(fuzzy_equals(bary2d, vec3(0.5f, 0.5f, 0.0f)));
}

TEST(Core_Math, PointContainment)
{
	EXPECT_TRUE(point_in_box(vec3(0.5f), vec3(0.0f), vec3(1.0f)));
	EXPECT_FALSE(point_in_box(vec3(1.5f, 0.5f, 0.5f), vec3(0.0f), vec3(1.0f)));
	EXPECT_TRUE(point_in_rectangle(vec2(0.5f, 0.5f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f)));
	EXPECT_FALSE(point_in_rectangle(vec2(-0.5f, 0.5f), vec2(0.0f, 0.0f), vec2(1.0f, 1.0f)));
}

TEST(Core_Math, Hashing)
{
	const vec3 a(1.0f, 2.0f, 3.0f);
	const vec3 b(1.0f, 2.0f, 3.0f);
	const vec3 c(3.0f, 2.0f, 1.0f);

	EXPECT_EQ(std::hash<vec3>()(a), std::hash<vec3>()(b));
	EXPECT_NE(std::hash<vec3>()(a), std::hash<vec3>()(c));
	EXPECT_EQ(std::hash<quat>()(quat::identity), std::hash<quat>()(quat(0.0f, 0.0f, 0.0f, 1.0f)));
	EXPECT_EQ(std::hash<mat4>()(mat4::identity), std::hash<mat4>()(mat4::identity));

	size_t first_seed = 0;
	hash_combine(first_seed, 1);
	hash_combine(first_seed, 2);

	size_t second_seed = 0;
	hash_combine(second_seed, 2);
	hash_combine(second_seed, 1);

	EXPECT_NE(first_seed, second_seed);
}

TEST(Core_Random, SeededGeneratorIsDeterministic)
{
	RandomNumberGenerator first(12345);
	RandomNumberGenerator second(12345);
	RandomNumberGenerator other(54321);

	bool differs = false;
	for (int32 i = 0; i < 100; ++i)
	{
		const uint64 value = first.random_uint64();
		EXPECT_EQ(value, second.random_uint64());
		differs |= value != other.random_uint64();
	}

	EXPECT_TRUE(differs);
}

TEST(Core_Random, ValuesStayInRange)
{
	RandomNumberGenerator rng(42);

	for (int32 i = 0; i < 1000; ++i)
	{
		const uint32 value = rng.random_uint32_between(10, 20);
		EXPECT_GE(value, 10u);
		EXPECT_LT(value, 20u);

		const float f = rng.random_float01();
		EXPECT_GE(f, 0.0f);
		EXPECT_LE(f, 1.0f);

		const float between = rng.random_float_between(-3.0f, 5.0f);
		EXPECT_GE(between, -3.0f);
		EXPECT_LE(between, 5.0f);

		const vec3 point = rng.random_point_on_unit_sphere();
		EXPECT_NEAR(length(point), 1.0f, 1e-3f);

		const quat rotation = rng.randomRotation();
		EXPECT_NEAR(length(rotation.v4), 1.0f, 1e-3f);
	}
}

TEST(Core_BoundingVolumes, BoundingBox)
{
	bounding_box box = bounding_box::negativeInfinity();
	box.grow(vec3(1.0f, -2.0f, 0.0f));
	box.grow(vec3(-1.0f, 2.0f, 4.0f));

	EXPECT_EQ(box.minCorner, vec3(-1.0f, -2.0f, 0.0f));
	EXPECT_EQ(box.maxCorner, vec3(1.0f, 2.0f, 4.0f));
	EXPECT_EQ(box.getCenter(), vec3(0.0f, 0.0f, 2.0f));
	EXPECT_EQ(box.getRadius(), vec3(1.0f, 2.0f, 2.0f));
	EXPECT_FLOAT_EQ(box.volume(), 32.0f);

	EXPECT_TRUE(box.contains(vec3(0.0f, 0.0f, 2.0f)));
	EXPECT_TRUE(box.contains(box.maxCorner));
	EXPECT_FALSE(box.contains(vec3(0.0f, 0.0f, 5.0f)));

	box.pad(vec3(1.0f));
	EXPECT_EQ(box.minCorner, vec3(-2.0f, -3.0f, -1.0f));
	EXPECT_EQ(box.maxCorner, vec3(2.0f, 3.0f, 5.0f));

	const bounding_box from_center = bounding_box::fromCenterRadius(vec3(1.0f), vec3(0.5f));
	EXPECT_EQ(from_center.minCorner, vec3(0.5f));
	EXPECT_EQ(from_center.maxCorner, vec3(1.5f));

	const bounding_box rotated = bounding_box::fromMinMax(vec3(-1.0f, -2.0f, -3.0f), vec3(1.0f, 2.0f, 3.0f))
		.transformToAABB(quat(vec3(0.0f, 1.0f, 0.0f), M_PI_OVER_2), vec3(10.0f, 0.0f, 0.0f));
	EXPECT_TRUE(fuzzy_equals(rotated.minCorner, vec3(7.0f, -2.0f, -1.0f)));
	EXPECT_TRUE(fuzzy_equals(rotated.maxCorner, vec3(13.0f, 2.0f, 1.0f)));
}

TEST(Core_BoundingVolumes, BoundingRectangle)
{
	bounding_rectangle rectangle = bounding_rectangle::negativeInfinity();
	rectangle.grow(vec2(1.0f, 1.0f));
	rectangle.grow(vec2(-1.0f, 3.0f));

	EXPECT_EQ(rectangle.getCenter(), vec2(0.0f, 2.0f));
	EXPECT_EQ(rectangle.getRadius(), vec2(1.0f, 1.0f));
	EXPECT_TRUE(rectangle.contains(vec2(0.0f, 2.0f)));
	EXPECT_FALSE(rectangle.contains(vec2(0.0f, 0.0f)));
}

TEST(Core_BoundingVolumes, RayIntersections)
{
	era_engine::ray r;
	r.origin = vec3(0.0f, 0.0f, -10.0f);
	r.direction = vec3(0.0f, 0.0f, 1.0f);

	float t = 0.0f;
	EXPECT_TRUE(r.intersectSphere(vec3(0.0f), 1.0f, t));
	EXPECT_NEAR(t, 9.0f, tolerance);
	EXPECT_FALSE(r.intersectSphere(vec3(5.0f, 0.0f, 0.0f), 1.0f, t));

	EXPECT_TRUE(r.intersectAABB(bounding_box::fromCenterRadius(vec3(0.0f), vec3(2.0f)), t));
	EXPECT_NEAR(t, 8.0f, tolerance);
	EXPECT_FALSE(r.intersectAABB(bounding_box::fromCenterRadius(vec3(0.0f, 5.0f, 0.0f), vec3(1.0f)), t));

	EXPECT_TRUE(r.intersectPlane(vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, 3.0f), t));
	EXPECT_NEAR(t, 13.0f, tolerance);

	bool front_facing = false;
	EXPECT_TRUE(r.intersectTriangle(vec3(-1.0f, -1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), t, front_facing));
	EXPECT_NEAR(t, 10.0f, tolerance);
	EXPECT_FALSE(r.intersectTriangle(vec3(2.0f, 2.0f, 0.0f), vec3(3.0f, 2.0f, 0.0f), vec3(2.0f, 3.0f, 0.0f), t, front_facing));
}
