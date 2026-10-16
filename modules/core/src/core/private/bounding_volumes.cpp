// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/bounding_volumes.h"
#include "core/collision_gjk.h"

namespace era_engine
{
	static bool point_in_aabb(vec3 point, BoundingBox aabb)
	{
		return point.x >= aabb.min_corner.x
			&& point.y >= aabb.min_corner.y
			&& point.z >= aabb.min_corner.z
			&& point.x <= aabb.max_corner.x
			&& point.y <= aabb.max_corner.y
			&& point.z <= aabb.max_corner.z;
	}

	static float closest_point_on_line_segment(vec3 point, vec3 line_a, vec3 line_b)
	{
		vec3 ab = line_b - line_a;
		float t = dot(point - line_a, ab) / squared_length(ab);
		t = clamp(t, 0.f, 1.f);
		return t;
	}

	void BoundingBox::grow(vec3 o)
	{
		min_corner.x = min(min_corner.x, o.x);
		min_corner.y = min(min_corner.y, o.y);
		min_corner.z = min(min_corner.z, o.z);
		max_corner.x = max(max_corner.x, o.x);
		max_corner.y = max(max_corner.y, o.y);
		max_corner.z = max(max_corner.z, o.z);
	}

	void BoundingBox::pad(vec3 p)
	{
		min_corner -= p;
		max_corner += p;
	}

	vec3 BoundingBox::get_center() const
	{
		return (min_corner + max_corner) * 0.5f;
	}

	vec3 BoundingBox::get_radius() const
	{
		return (max_corner - min_corner) * 0.5f;
	}

	bool BoundingBox::contains(vec3 p) const
	{
		return p.x >= min_corner.x && p.x <= max_corner.x
			&& p.y >= min_corner.y && p.y <= max_corner.y
			&& p.z >= min_corner.z && p.z <= max_corner.z;
	}

	BoundingBox BoundingBox::transform_to_aabb(quat rotation, vec3 translation) const
	{
		BoundingBox result = BoundingBox::negative_infinity();
		result.grow(rotation * min_corner + translation);
		result.grow(rotation * vec3(max_corner.x, min_corner.y, min_corner.z) + translation);
		result.grow(rotation * vec3(min_corner.x, max_corner.y, min_corner.z) + translation);
		result.grow(rotation * vec3(max_corner.x, max_corner.y, min_corner.z) + translation);
		result.grow(rotation * vec3(min_corner.x, min_corner.y, max_corner.z) + translation);
		result.grow(rotation * vec3(max_corner.x, min_corner.y, max_corner.z) + translation);
		result.grow(rotation * vec3(min_corner.x, max_corner.y, max_corner.z) + translation);
		result.grow(rotation * max_corner + translation);
		return result;
	}

	BoundingOrientedBox BoundingBox::transform_to_obb(quat rotation, vec3 translation) const
	{
		BoundingOrientedBox obb;
		obb.center = rotation * get_center() + translation;
		obb.radius = get_radius();
		obb.rotation = rotation;
		return obb;
	}

	BoundingBoxCorners BoundingBox::get_corners() const
	{
		BoundingBoxCorners result;
		result.i = min_corner;
		result.x = vec3(max_corner.x, min_corner.y, min_corner.z);
		result.y = vec3(min_corner.x, max_corner.y, min_corner.z);
		result.xy = vec3(max_corner.x, max_corner.y, min_corner.z);
		result.z = vec3(min_corner.x, min_corner.y, max_corner.z);
		result.xz = vec3(max_corner.x, min_corner.y, max_corner.z);
		result.yz = vec3(min_corner.x, max_corner.y, max_corner.z);
		result.xyz = max_corner;
		return result;
	}

	BoundingBoxCorners BoundingBox::get_corners(quat rotation, vec3 translation) const
	{
		BoundingBoxCorners result;
		result.i = rotation * min_corner + translation;
		result.x = rotation * vec3(max_corner.x, min_corner.y, min_corner.z) + translation;
		result.y = rotation * vec3(min_corner.x, max_corner.y, min_corner.z) + translation;
		result.xy = rotation * vec3(max_corner.x, max_corner.y, min_corner.z) + translation;
		result.z = rotation * vec3(min_corner.x, min_corner.y, max_corner.z) + translation;
		result.xz = rotation * vec3(max_corner.x, min_corner.y, max_corner.z) + translation;
		result.yz = rotation * vec3(min_corner.x, max_corner.y, max_corner.z) + translation;
		result.xyz = rotation * max_corner + translation;
		return result;
	}

	BoundingBox BoundingBox::everything()
	{
		return BoundingBox{ vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX), vec3(FLT_MAX, FLT_MAX, FLT_MAX) };
	}

	BoundingBox BoundingBox::negative_infinity()
	{
		return BoundingBox{ vec3(FLT_MAX, FLT_MAX, FLT_MAX), vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX) };
	}

	BoundingBox BoundingBox::from_min_max(vec3 min_corner, vec3 max_corner)
	{
		return BoundingBox{ min_corner, max_corner };
	}

	BoundingBox BoundingBox::from_center_radius(vec3 center, vec3 radius)
	{
		return BoundingBox{ center - radius, center + radius };
	}

	BoundingBox BoundingOrientedBox::get_aabb() const
	{
		BoundingBox bb = { -radius, radius };
		return bb.transform_to_aabb(rotation, center);
	}

	BoundingBox BoundingOrientedBox::transform_to_aabb(quat rotation, vec3 translation) const
	{
		BoundingBox bb = { -radius, radius };
		return bb.transform_to_aabb(rotation * this->rotation, rotation * center + translation);
	}

	BoundingOrientedBox BoundingOrientedBox::transform_to_obb(quat rotation, vec3 translation) const
	{
		return BoundingOrientedBox{ rotation * this->rotation, rotation * center + translation, radius };
	}

	BoundingBoxCorners BoundingOrientedBox::get_corners() const
	{
		BoundingBox bb = { -radius, radius };
		return bb.get_corners(rotation, center);
	}

	void BoundingRectangle::grow(vec2 o)
	{
		min_corner.x = min(min_corner.x, o.x);
		min_corner.y = min(min_corner.y, o.y);
		max_corner.x = max(max_corner.x, o.x);
		max_corner.y = max(max_corner.y, o.y);
	}

	void BoundingRectangle::pad(vec2 p)
	{
		min_corner -= p;
		max_corner += p;
	}

	vec2 BoundingRectangle::get_center() const
	{
		return (min_corner + max_corner) * 0.5f;
	}

	vec2 BoundingRectangle::get_radius() const
	{
		return (max_corner - min_corner) * 0.5f;
	}

	bool BoundingRectangle::contains(vec2 p) const
	{
		return p.x >= min_corner.x && p.x <= max_corner.x
			&& p.y >= min_corner.y && p.y <= max_corner.y;
	}

	BoundingRectangle BoundingRectangle::negative_infinity()
	{
		return BoundingRectangle{ vec2(FLT_MAX, FLT_MAX), vec2(-FLT_MAX, -FLT_MAX) };
	}

	BoundingRectangle BoundingRectangle::from_min_max(vec2 min_corner, vec2 max_corner)
	{
		return BoundingRectangle{ min_corner, max_corner };
	}

	BoundingRectangle BoundingRectangle::from_center_radius(vec2 center, vec2 radius)
	{
		return BoundingRectangle{ center - radius, center + radius };
	}

	bool ray::intersect_plane(vec3 normal, float d, float& out_t) const
	{
		float ndotd = dot(direction, normal);
		if (abs(ndotd) < 1e-6f)
		{
			return false;
		}

		out_t = -(dot(origin, normal) + d) / ndotd;
		return true;
	}

	bool ray::intersect_plane(vec3 normal, vec3 point, float& out_t) const
	{
		float d = -dot(normal, point);
		return intersect_plane(normal, d, out_t);
	}

	bool ray::intersect_aabb(const BoundingBox& a, float& out_t) const
	{
		vec3 inv_dir = vec3(1.f / direction.x, 1.f / direction.y, 1.f / direction.z); // This can be Inf (when one direction component is 0) but still works

		float tx1 = (a.min_corner.x - origin.x) * inv_dir.x;
		float tx2 = (a.max_corner.x - origin.x) * inv_dir.x;

		out_t = min(tx1, tx2);
		float tmax = max(tx1, tx2);

		float ty1 = (a.min_corner.y - origin.y) * inv_dir.y;
		float ty2 = (a.max_corner.y - origin.y) * inv_dir.y;

		out_t = max(out_t, min(ty1, ty2));
		tmax = min(tmax, max(ty1, ty2));

		float tz1 = (a.min_corner.z - origin.z) * inv_dir.z;
		float tz2 = (a.max_corner.z - origin.z) * inv_dir.z;

		out_t = max(out_t, min(tz1, tz2));
		tmax = min(tmax, max(tz1, tz2));

		bool result = tmax >= out_t && out_t > 0.f;

		return result;
	}

	bool ray::intersect_obb(const BoundingOrientedBox& a, float& out_t) const
	{
		//return (conjugate(m.rotation) * (pos - m.position)) / m.scale;
		ray local_r = { conjugate(a.rotation) * (origin - a.center), conjugate(a.rotation) * direction };
		return local_r.intersect_aabb(BoundingBox::from_center_radius(0.f, a.radius), out_t);
	}

	bool ray::intersect_triangle(vec3 a, vec3 b, vec3 c, float& out_t, bool& out_front_facing) const
	{
		vec3 normal = noz(cross(b - a, c - a));
		float d = -dot(normal, a);

		float n_dot_r = dot(direction, normal);
		if (fabsf(n_dot_r) <= 1e-6f)
			return false;

		out_t = -(dot(origin, normal) + d) / n_dot_r;

		vec3 q = origin + out_t * direction;
		out_front_facing = n_dot_r < 0.f;
		return out_t >= 0.f && point_in_triangle(q, a, b, c);
	}

	bool ray::intersect_sphere(vec3 center, float radius, float& out_t) const
	{
		vec3 m = origin - center;
		float b = dot(m, direction);
		float c = dot(m, m) - radius * radius;

		if (c > 0.f && b > 0.f)
			return false;

		float discr = b * b - c;

		if (discr < 0.f)
			return false;

		out_t = -b - sqrt(discr);

		if (out_t < 0.f)
			out_t = 0.f;

		return true;
	}

	bool ray::intersect_cylinder(const BoundingCylinder& cylinder, float& out_t) const
	{
		vec3 d = direction;
		vec3 o = origin;

		vec3 axis = cylinder.position_b - cylinder.position_a;
		float height = length(axis);

		quat q = rotate_from_to(axis, vec3(0.f, 1.f, 0.f));

		// Cylinder base is at 0,0,0. Second point is above it
		o = q * (o - cylinder.position_a);
		d = q * d;

		float epsilon = 1e-6f;

		float y = -1.f;
		if (squared_length(vec2(o.x, o.z)) > cylinder.radius * cylinder.radius)
		{
			// We are outside the infinite cylinder. Only in this case it is possible to hit the sides

			float a = d.x * d.x + d.z * d.z;
			float b = d.x * o.x + d.z * o.z;
			float c = o.x * o.x + o.z * o.z - cylinder.radius * cylinder.radius;

			float delta = b * b - a * c;

			if (delta < epsilon)
				return false;

			out_t = (-b - sqrt(delta)) / a;
			if (out_t <= epsilon)
				return false; // Behind ray

			y = o.y + out_t * d.y;
		}

		// Check bases. Always true if the above if is not taken
		if (y > height + epsilon || y < -epsilon)
		{
			vec3 pos_a = vec3(0.f, 0.f, 0.f);
			vec3 pos_b = vec3(0.f, height, 0.f);

			ray local_ray = { o, d };

			float dist;
			bool b1 = d.y < 0.f && local_ray.intersect_disk(pos_b, vec3(0.f, 1.f, 0.f), cylinder.radius, dist);
			if (b1)
				out_t = dist;
			bool b2 = d.y > 0.f && local_ray.intersect_disk(pos_a, vec3(0.f, -1.f, 0.f), cylinder.radius, dist);
			if (b2)
				out_t = dist;

			y = o.y + out_t * d.y;
		}

		return y > -epsilon && y < height + epsilon;
	}

	bool ray::intersect_capsule(const BoundingCapsule& capsule, float& out_t) const
	{
		out_t = FLT_MAX;
		float t;
		bool result = false;
		if (intersect_cylinder(BoundingCylinder{ capsule.position_a, capsule.position_b, capsule.radius }, t))
		{
			out_t = t;
			result = true;
		}
		if (intersect_sphere(BoundingSphere{ capsule.position_a, capsule.radius }, t))
		{
			out_t = min(out_t, t);
			result = true;
		}
		if (intersect_sphere(BoundingSphere{ capsule.position_b, capsule.radius }, t))
		{
			out_t = min(out_t, t);
			result = true;
		}
		return result;
	}

	bool ray::intersect_disk(vec3 pos, vec3 normal, float radius, float& out_t) const
	{
		bool intersects_plane = intersect_plane(normal, pos, out_t);
		if (intersects_plane)
			return length(origin + out_t * direction - pos) <= radius;
		return false;
	}

	bool ray::intersect_rectangle(vec3 pos, vec3 tangent, vec3 bitangent, vec2 radius, float& out_t) const
	{
		vec3 normal = cross(tangent, bitangent);
		bool intersects_plane = intersect_plane(normal, pos, out_t);
		if (intersects_plane)
		{
			vec3 offset = origin + out_t * direction - pos;
			vec2 projected(dot(offset, tangent), dot(offset, bitangent));
			projected = abs(projected);
			if (projected.x <= radius.x && projected.y <= radius.y)
				return true;
		}
		return false;
	}

	static bool is_zero(float x)
	{
		return abs(x) < 1e-6f;
	}

	struct Solve2Result
	{
		uint32 num_results;
		float results[2];
	};

	struct Solve3Result
	{
		uint32 num_results;
		float results[3];
	};

	struct Solve4Result
	{
		uint32 num_results;
		float results[4];
	};

	static Solve2Result solve2(float c0, float c1, float c2)
	{
		float p = c1 / (2 * c2);
		float q = c0 / c2;

		float D = p * p - q;

		if (is_zero(D))
			return { 1, -p };
		else if (D < 0)
			return { 0 };
		else /* if (D > 0) */
		{
			float sqrt_d = sqrt(D);

			return { 2, sqrt_d - p, -sqrt_d - p };
		}
	}

	static Solve3Result solve3(float c0, float c1, float c2, float c3)
	{
		float A = c2 / c3;
		float B = c1 / c3;
		float C = c0 / c3;

		float sq_a = A * A;
		float p = 1.f / 3 * (-1.f / 3 * sq_a + B);
		float q = 1.f / 2 * (2.f / 27 * A * sq_a - 1.f / 3 * A * B + C);

		/* use Cardano's formula */

		float cb_p = p * p * p;
		float D = q * q + cb_p;

		Solve3Result s = {};

		if (is_zero(D))
		{
			if (is_zero(q))
				s = { 1, 0.f };
			else
			{
				float u = cbrt(-q);
				s = { 2, 2.f * u, -u };
			}
		}
		else if (D < 0) /* Casus irreducibilis: three real solutions */
		{
			float phi = 1.f / 3.f * acos(-q / sqrt(-cb_p));
			float t = 2.f * sqrt(-p);

			s = { 3,
				t * cos(phi),
				-t * cos(phi + M_PI / 3),
				-t * cos(phi - M_PI / 3) };

		}
		else /* one real solution */
		{
			float sqrt_d = sqrt(D);
			float u = cbrt(sqrt_d - q);
			float v = -cbrt(sqrt_d + q);

			s = { 1, u + v };
		}

		/* resubstitute */
		float sub = 1.f / 3.f * A;

		for (uint32 i = 0; i < s.num_results; ++i)
			s.results[i] -= sub;

		return s;
	}

	/**
	 *  Solves equation:
	 *
	 *      c[0] + c[1]*x + c[2]*x^2 + c[3]*x^3 + c[4]*x^4 = 0
	 *
	 */
	static Solve4Result solve4(float c0, float c1, float c2, float c3, float c4)
	{
		/* normal form: x^4 + Ax^3 + Bx^2 + Cx + D = 0 */

		float A = c3 / c4;
		float B = c2 / c4;
		float C = c1 / c4;
		float D = c0 / c4;

		/*  substitute x = y - A/4 to eliminate cubic term:
		x^4 + px^2 + qx + r = 0 */

		float sq_a = A * A;
		float p = -3.f / 8 * sq_a + B;
		float q = 1.f / 8 * sq_a * A - 1.f / 2 * A * B + C;
		float r = -3.f / 256 * sq_a * sq_a + 1.f / 16 * sq_a * B - 1.f / 4 * A * C + D;
		Solve4Result s = {};

		if (is_zero(r))
		{
			/* no absolute term: y(y^3 + py + q) = 0 */

			auto s3 = solve3(q, p, 0, 1);
			for (uint32 i = 0; i < s3.num_results; ++i)
				s.results[s.num_results++] = s3.results[i];

			s.results[s.num_results++] = 0.f;
		}
		else
		{
			/* solve the resolvent cubic ... */
			auto s3 = solve3(1.f / 2 * r * p - 1.f / 8 * q * q, -r, -0.5f * p, 1.f);
			for (uint32 i = 0; i < s3.num_results; ++i)
				s.results[s.num_results++] = s3.results[i];

			/* ... and take the one real solution ... */
			float z = s.results[0];

			/* ... to build two quadric equations */
			float u = z * z - r;
			float v = 2.f * z - p;

			if (is_zero(u))
				u = 0;
			else if (u > 0)
				u = sqrt(u);
			else
				return {};

			if (is_zero(v))
				v = 0;
			else if (v > 0)
				v = sqrt(v);
			else
				return {};

			auto s2 = solve2(z - u, q < 0 ? -v : v, 1);
			s = {};
			for (uint32 i = 0; i < s2.num_results; ++i)
				s.results[s.num_results++] = s2.results[i];

			s2 = solve2(z + u, q < 0 ? v : -v, 1);
			for (uint32 i = 0; i < s2.num_results; ++i)
				s.results[s.num_results++] = s2.results[i];
		}

		/* resubstitute */
		float sub = 1.f / 4 * A;

		for (uint32 i = 0; i < s.num_results; ++i)
			s.results[i] -= sub;

		return s;
	}

	bool ray::intersect_torus(const BoundingTorus& torus, float& out_t) const
	{
		vec3 d = direction;
		vec3 o = origin;

		vec3 axis = torus.up_axis;

		quat q = rotate_from_to(axis, vec3(0.f, 1.f, 0.f));

		o = q * (o - torus.position);
		d = q * d;

		// define the coefficients of the quartic equation
		float sum_d_sqrd = dot(d, d);

		float e = dot(o, o) - torus.major_radius * torus.major_radius - torus.tube_radius * torus.tube_radius;
		float f = dot(o, d);
		float four_a_sqrd = 4.f * torus.major_radius * torus.major_radius;

		auto solution = solve4(
			e * e - four_a_sqrd * (torus.tube_radius * torus.tube_radius - o.y * o.y),
			4.f * f * e + 2.f * four_a_sqrd * o.y * d.y,
			2.f * sum_d_sqrd * e + 4.f * f * f + four_a_sqrd * d.y * d.y,
			4.f * sum_d_sqrd * f,
			sum_d_sqrd * sum_d_sqrd
		);

		// ray misses the torus
		if (solution.num_results == 0)
			return false;

		// find the smallest root greater than kEpsilon, if any
		// the roots array is not sorted
		float min_t = FLT_MAX;
		for (uint32 i = 0; i < solution.num_results; ++i)
		{
			float t = solution.results[i];
			if ((t > 1e-6f) && (t < min_t))
				min_t = t;
		}
		out_t = min_t;

		return true;
	}

	bool ray::intersect_hull(const BoundingHull& hull, const BoundingHullGeometry& geometry, float& out_t) const
	{
		ray local_r = { conjugate(hull.rotation) * (origin - hull.position), conjugate(hull.rotation) * direction };

		float min_t = FLT_MAX;
		bool result = false;

		for (uint32 i = 0; i < (uint32)geometry.faces.size(); ++i)
		{
			auto tri = geometry.faces[i];
			vec3 a = geometry.vertices[tri.a];
			vec3 b = geometry.vertices[tri.b];
			vec3 c = geometry.vertices[tri.c];

			float t;
			bool ff;
			if (local_r.intersect_triangle(a, b, c, t, ff) && t < min_t)
			{
				min_t = t;
				result = true;
			}
		}

		out_t = min_t;
		return result;
	}

	bool sphere_vs_cylinder(const BoundingSphere& s, const BoundingCylinder& c)
	{
		vec3 ab = c.position_b - c.position_a;
		float t = dot(s.center - c.position_a, ab) / squared_length(ab);
		if (t >= 0.f && t <= 1.f)
			return sphere_vs_sphere(s, BoundingSphere{ lerp(c.position_a, c.position_b, t), c.radius });

		vec3 p = (t <= 0.f) ? c.position_a : c.position_b;
		vec3 up = (t <= 0.f) ? -ab : ab;

		vec3 projected_dir_to_center = normalize(cross(cross(up, s.center - p), up));
		vec3 end_a = p + projected_dir_to_center * c.radius;
		vec3 end_b = p - projected_dir_to_center * c.radius;

		vec3 closest_to_sphere = closest_point_point_segment(s.center, LineSegment{ end_a, end_b });
		float sq_distance = squared_length(closest_to_sphere - s.center);

		return sq_distance <= s.radius;
	}

	bool sphere_vs_hull(const BoundingSphere& s, const BoundingHull& h)
	{
		SphereSupportFn sphere_support{ s };
		HullSupportFn hull_support{ h };

		GJKSimplex gjk_simplex;
		return gjk_intersection_test(sphere_support, hull_support, gjk_simplex);
	}

	bool sphere_vs_triangle(const BoundingSphere& s, vec3 a, vec3 b, vec3 c)
	{
		vec3 p = closest_point_point_triangle(s.center, a, b, c);
		vec3 v = p - s.center;
		return dot(v, v) <= s.radius * s.radius;
	}

	bool capsule_vs_aabb(const BoundingCapsule& c, const BoundingBox& b)
	{
		CapsuleSupportFn capsule_support{ c };
		AABBSupportFn box_support{ b };

		GJKSimplex gjk_simplex;
		return gjk_intersection_test(capsule_support, box_support, gjk_simplex);
	}

	bool capsule_vs_obb(const BoundingCapsule& c, const BoundingOrientedBox& o)
	{
		BoundingBox aabb = BoundingBox::from_center_radius(o.center, o.radius);
		BoundingCapsule c_ = {
			conjugate(o.rotation) * (c.position_a - o.center) + o.center,
			conjugate(o.rotation) * (c.position_b - o.center) + o.center,
			c.radius };

		return capsule_vs_aabb(c_, aabb);
	}

	bool capsule_vs_hull(const BoundingCapsule& c, const BoundingHull& h)
	{
		CapsuleSupportFn capsule_support{ c };
		HullSupportFn hull_support{ h };

		GJKSimplex gjk_simplex;
		return gjk_intersection_test(capsule_support, hull_support, gjk_simplex);
	}

	bool capsule_vs_triangle(const BoundingCapsule& capsule, vec3 a, vec3 b, vec3 c)
	{
		ray r = { capsule.position_a, normalize(capsule.position_b - capsule.position_a) };

		vec3 tri_normal = normalize(cross(b - a, c - a));
		float d = -dot(tri_normal, a);

		float ndotd = dot(r.direction, tri_normal);

		float t = -(dot(r.origin, tri_normal) + d) / ndotd;

		vec3 trace = r.origin + t * r.direction;
		vec3 closest = closest_point_point_triangle(trace, a, b, c);

		vec3 reference = closest_point_point_segment(closest, { capsule.position_a, capsule.position_b });

		return sphere_vs_triangle(BoundingSphere{ reference, capsule.radius }, a, b, c);
	}

	bool cylinder_vs_cylinder(const BoundingCylinder& a, const BoundingCylinder& b)
	{
		CylinderSupportFn cylinder_support_a{ a };
		CylinderSupportFn cylinder_support_b{ b };

		GJKSimplex gjk_simplex;
		return gjk_intersection_test(cylinder_support_a, cylinder_support_b, gjk_simplex);
	}

	bool cylinder_vs_aabb(const BoundingCylinder& c, const BoundingBox& b)
	{
		CylinderSupportFn cylinder_support{ c };
		AABBSupportFn box_support{ b };

		GJKSimplex gjk_simplex;
		return gjk_intersection_test(cylinder_support, box_support, gjk_simplex);
	}

	bool cylinder_vs_obb(const BoundingCylinder& c, const BoundingOrientedBox& o)
	{
		BoundingBox aabb = BoundingBox::from_center_radius(o.center, o.radius);
		BoundingCylinder c_ = {
			conjugate(o.rotation) * (c.position_a - o.center) + o.center,
			conjugate(o.rotation) * (c.position_b - o.center) + o.center,
			c.radius };

		return cylinder_vs_aabb(c_, aabb);
	}

	bool cylinder_vs_hull(const BoundingCylinder& c, const BoundingHull& h)
	{
		CylinderSupportFn cylinder_support{ c };
		HullSupportFn hull_support{ h };

		GJKSimplex gjk_simplex;
		return gjk_intersection_test(cylinder_support, hull_support, gjk_simplex);
	}

	bool aabb_vs_hull(const BoundingBox& a, const BoundingHull& h)
	{
		AABBSupportFn aabb_support{ a };
		HullSupportFn hull_support{ h };

		GJKSimplex gjk_simplex;
		return gjk_intersection_test(aabb_support, hull_support, gjk_simplex);
	}

	bool aabb_vs_plane(const BoundingBox& a, vec4 plane)
	{
		vec3 center = a.get_center();
		vec3 radius = a.get_radius();

		float r = dot(radius, abs(plane.xyz));
		float s = abs(signed_distance_to_plane(center, plane));

		return s <= r;
	}

	bool aabb_vs_triangle(const BoundingBox& aabb, vec3 a, vec3 b, vec3 c)
	{
		vec3 radius = aabb.get_radius();
		vec3 center = aabb.get_center();

		a -= center;
		b -= center;
		c -= center;

		vec3 f0 = b - a;
		vec3 f1 = c - b;
		vec3 f2 = a - c;

		// Test axis (1, 0, 0) x f0 = (0, -f0.z, f0.y)
		{
			// p0	= dot(a, axis)
			//		= (-a.y * f0.z) + (a.z * f0.y)
			//		= -a.y * (b.z - a.z) + a.z * (b.y - a.y)
			//		= (-a.y * b.z) + (a.y * a.z) + (a.z * b.y) - (a.z * a.y)
			//		= (-a.y * b.z) + (a.z * b.y)

			// p1	= dot(b, axis)
			//		= (-b.y * f0.z) + (b.z * f0.y)
			//		= -b.y * (b.z - a.z) + b.z * (b.y - a.y)
			//		= (-b.y * b.z) + (b.y - a.z) + (b.z * b.y) - (b.z * a.y)
			//		= (b.y - a.z) - (b.z * a.y)
			//		= p0

			// p2	= dot(c, axis)
			//		= (-c.y * f0.z) + (c.z * f0.y)

			float p0 = (a.z * f0.y) - (a.y * f0.z);
			float p2 = (c.z * f0.y) - (c.y * f0.z);
			float r = radius.y * abs(f0.z) + radius.z * abs(f0.y);
			if (max(-max(p0, p2), min(p0, p2)) > r)
				return false;
		}

		// Test axis (1, 0, 0) x f1 = (0, -f1.z, f1.y)
		{
			// p0	= dot(a, axis)
			//		= (-a.y * f1.z) + (a.z * f1.y)
			//		= -a.y * (c.z - b.z) + a.z * (c.y - b.y)
			//		= (-a.y * c.z) + (a.y * b.z) + (a.z * c.y) - (a.z * b.y)

			// p1	= dot(b, axis)
			//		= (-b.y * f1.z) + (b.z * f1.y)
			//		= -b.y * (c.z - b.z) + b.z * (c.y - b.y)
			//		= (-b.y * c.z) + (b.y * b.z) + (b.z * c.y) - (b.z * b.y)
			//		= (-b.y * c.z) + (b.z * c.y)

			// p2	= dot(c, axis)
			//		= (-c.y * f1.z) + (c.z * f1.y)
			//		= -c.y * (c.z - b.z) + c.z * (c.y - b.y)
			//		= (-c.y * c.z) + (c.y * b.z) + (c.z * c.y) - (c.z * b.y)
			//		= (c.y * b.z) - (c.z * b.y)
			//		= p1

			float p0 = (a.z * f1.y) - (a.y * f1.z);
			float p1 = (b.z * f1.y) - (b.y * f1.z);
			float r = radius.y * abs(f1.z) + radius.z * abs(f1.y);
			if (max(-max(p0, p1), min(p0, p1)) > r)
				return false;
		}

		// Test axis (1, 0, 0) x f2 = (0, -f2.z, f2.y)
		{
			// p0	= dot(a, axis)
			//		= (-a.y * f2.z) + (a.z * f2.y)
			//		= -a.y * (a.z - c.z) + a.z * (a.y - c.y)
			//		= (-a.y * a.z) + (a.y * c.z) + (a.z * a.y) - (a.z * c.y)
			//		= (a.y * c.z) - (a.z * c.y)

			// p1	= dot(b, axis)
			//		= (-b.y * f2.z) + (b.z * f2.y)
			//		= -b.y * (a.z - c.z) + b.z * (a.y - c.y)
			//		= (-b.y * a.z) + (b.y * c.z) + (b.z * a.y) - (b.z * c.y)

			// p2	= dot(c, axis)
			//		= (-c.y * f2.z) + (c.z * f2.y)
			//		= -c.y * (a.z - c.z) + c.z * (a.y - c.y)
			//		= (-c.y * a.z) + (c.y * c.z) + (c.z * a.y) - (c.z * c.y)
			//		= (-c.y * a.z) + (c.z * a.y)
			//		= p0

			float p0 = (a.z * f2.y) - (a.y * f2.z);
			float p1 = (b.z * f2.y) - (b.y * f2.z);
			float r = radius.y * abs(f2.z) + radius.z * abs(f2.y);
			if (max(-max(p0, p1), min(p0, p1)) > r)
				return false;
		}


		// Test axis (0, 1, 0) x f0 = (f0.z, 0, -f0.x)
		{
			// p0	= dot(a, axis)
			//		= (a.x * f0.z) - (a.z * f0.x)
			//		= a.x * (b.z - a.z) - a.z * (b.x - a.x)
			//		= (a.x * b.z) - (a.x * a.z) - (a.z * b.x) + (a.z * a.x)
			//		= (a.x * b.z) - (a.z * b.x)

			// p1	= dot(b, axis)
			//		= (b.x * f0.z) - (b.z * f0.x)
			//		= b.x * (b.z - a.z) - b.z * (b.x - a.x)
			//		= (b.x * b.z) - (b.x * a.z) - (b.z * b.x) + (b.z * a.x)
			//		= (b.z * a.x) - (b.x * a.z)
			//		= p0

			// p2	= dot(c, axis)
			//		= (c.x * f0.z) - (c.z * f0.x)

			float p0 = (a.x * f0.z) - (a.z * f0.x);
			float p2 = (c.x * f0.z) - (c.z * f0.x);
			float r = radius.x * abs(f0.z) + radius.z * abs(f0.x);
			if (max(-max(p0, p2), min(p0, p2)) > r)
				return false;
		}

		// Test axis (0, 1, 0) x f1 = (f1.z, 0, -f1.x)
		{
			// p0	= dot(a, axis)
			//		= (a.x * f1.z) - (a.z * f1.x)

			// p1	= dot(b, axis)
			//		= (b.x * f1.z) - (b.z * f1.x)

			// p2	= dot(c, axis)
			//		= (c.x * f1.z) - (c.z * f1.x)
			//		= p1

			float p0 = (a.x * f1.z) - (a.z * f1.x);
			float p1 = (b.x * f1.z) - (b.z * f1.x);
			float r = radius.x * abs(f1.z) + radius.z * abs(f1.x);
			if (max(-max(p0, p1), min(p0, p1)) > r)
				return false;
		}

		// Test axis (0, 1, 0) x f2 = (f2.z, 0, -f2.x)
		{
			// p0	= dot(a, axis)
			//		= (a.x * f2.z) - (a.z * f2.x)

			// p1	= dot(b, axis)
			//		= (b.x * f2.z) - (b.z * f2.x)

			// p2	= dot(c, axis)
			//		= (c.x * f2.z) - (c.z * f2.x)
			//		= p0

			float p0 = (a.x * f2.z) - (a.z * f2.x);
			float p1 = (b.x * f2.z) - (b.z * f2.x);
			float r = radius.x * abs(f2.z) + radius.z * abs(f2.x);
			if (max(-max(p0, p1), min(p0, p1)) > r)
				return false;
		}


		// Test axis (0, 0, 1) x f0 = (-f0.y, f0.x, 0)
		{
			// p0	= dot(a, axis)
			//		= (-a.x * f0.y) + (a.y * f0.x)

			// p1	= dot(b, axis)
			//		= (-b.x * f0.y) + (b.y * f0.x)
			//		= p0

			// p2	= dot(c, axis)
			//		= (-c.x * f0.y) + (c.y * f0.x)

			float p0 = (a.y * f0.x) - (a.x * f0.y);
			float p2 = (c.y * f0.x) - (c.x * f0.y);
			float r = radius.x * abs(f0.y) + radius.y * abs(f0.x);
			if (max(-max(p0, p2), min(p0, p2)) > r)
				return false;
		}

		// Test axis (0, 0, 1) x f1 = (-f1.y, f1.x, 0)
		{
			// p0	= dot(a, axis)
			//		= (-a.x * f1.y) + (a.y * f1.x)

			// p1	= dot(b, axis)
			//		= (-b.x * f1.y) + (b.y * f1.x)

			// p2	= dot(c, axis)
			//		= (-c.x * f1.y) + (c.y * f1.x)
			//		= p1

			float p0 = (a.y * f1.x) - (a.x * f1.y);
			float p1 = (b.y * f1.x) - (b.x * f1.y);
			float r = radius.x * abs(f1.y) + radius.y * abs(f1.x);
			if (max(-max(p0, p1), min(p0, p1)) > r)
				return false;
		}

		// Test axis (0, 0, 1) x f2 = (-f2.y, f2.x, 0)
		{
			// p0	= dot(a, axis)
			//		= (-a.x * f2.y) + (a.y * f2.x)

			// p1	= dot(b, axis)
			//		= (-b.x * f2.y) + (b.y * f2.x)

			// p2	= dot(c, axis)
			//		= (-c.x * f2.y) + (c.y * f2.x)
			//		= p0

			float p0 = (a.y * f2.x) - (a.x * f2.y);
			float p1 = (b.y * f2.x) - (b.x * f2.y);
			float r = radius.x * abs(f2.y) + radius.y * abs(f2.x);
			if (max(-max(p0, p1), min(p0, p1)) > r)
				return false;
		}

		// Test face normals of AABB. Exit if...
		// ... [-radius.x, radius.x] and [min(a.x,b.x,c.x), max(a.x,b.x,c.x)] do not overlap
		if (max(a.x, max(b.x, c.x)) < -radius.x || min(a.x, min(b.x, c.x)) > radius.x)
			return false;
		// ... [-radius.y, radius.y] and [min(a.y,b.y,c.y), max(a.y,b.y,c.y)] do not overlap
		if (max(a.y, max(b.y, c.y)) < -radius.y || min(a.y, min(b.y, c.y)) > radius.y)
			return false;
		// ... [-radius.z, radius.z] and [min(a.z,b.z,c.z), max(a.z,b.z,c.z)] do not overlap
		if (max(a.z, max(b.z, c.z)) < -radius.z || min(a.z, min(b.z, c.z)) > radius.z)
			return false;

		{
			vec3 tri_normal = cross(f0, f1);
			float tri_d = dot(tri_normal, a);

			float r = dot(radius, abs(tri_normal));
			float s = abs(tri_d);

			if (s > r)
				return false;
		}

		return true;
	}

	bool obb_vs_obb(const BoundingOrientedBox& a, const BoundingOrientedBox& b)
	{
		union OBBAxes
		{
			struct
			{
				vec3 x, y, z;
			};
			vec3 u[3];
		};

		float ra, rb, penetration;

		OBBAxes axes_a = {
			a.rotation * vec3(1.f, 0.f, 0.f),
			a.rotation * vec3(0.f, 1.f, 0.f),
			a.rotation * vec3(0.f, 0.f, 1.f),
		};

		OBBAxes axes_b = {
			b.rotation * vec3(1.f, 0.f, 0.f),
			b.rotation * vec3(0.f, 1.f, 0.f),
			b.rotation * vec3(0.f, 0.f, 1.f),
		};

		mat3 r;
		r.m00 = dot(axes_a.x, axes_b.x);
		r.m10 = dot(axes_a.y, axes_b.x);
		r.m20 = dot(axes_a.z, axes_b.x);
		r.m01 = dot(axes_a.x, axes_b.y);
		r.m11 = dot(axes_a.y, axes_b.y);
		r.m21 = dot(axes_a.z, axes_b.y);
		r.m02 = dot(axes_a.x, axes_b.z);
		r.m12 = dot(axes_a.y, axes_b.z);
		r.m22 = dot(axes_a.z, axes_b.z);

		vec3 tw = b.center - a.center;
		vec3 t = conjugate(a.rotation) * tw;

		mat3 abs_r;
		for (uint32 i = 0; i < 9; ++i)
			abs_r.m[i] = abs(r.m[i]) + EPSILON; // Add in an epsilon term to counteract arithmetic errors when two edges are parallel and their cross product is (near) 0

		// Test a's faces
		for (uint32 i = 0; i < 3; ++i)
		{
			ra = a.radius.data[i];
			rb = dot(row(abs_r, i), b.radius);
			float d = t.data[i];
			penetration = ra + rb - abs(d);
			if (penetration < 0.f)
				return false;
		}

		// Test b's faces
		for (uint32 i = 0; i < 3; ++i)
		{
			ra = dot(col(abs_r, i), a.radius);
			rb = b.radius.data[i];
			float d = dot(col(r, i), t);
			penetration = ra + rb - abs(d);
			if (penetration < 0.f)
				return false;
		}

		// Test a.x x b.x
		ra = a.radius.y * abs_r.m20 + a.radius.z * abs_r.m10;
		rb = b.radius.y * abs_r.m02 + b.radius.z * abs_r.m01;
		penetration = ra + rb - abs(t.z * r.m10 - t.y * r.m20);
		if (penetration < 0.f)
			return false;

		// Test a.x x b.y.
		ra = a.radius.y * abs_r.m21 + a.radius.z * abs_r.m11;
		rb = b.radius.x * abs_r.m02 + b.radius.z * abs_r.m00;
		penetration = ra + rb - abs(t.z * r.m11 - t.y * r.m21);
		if (penetration < 0.f)
			return false;

		// Test a.x x b.z
		ra = a.radius.y * abs_r.m22 + a.radius.z * abs_r.m12;
		rb = b.radius.x * abs_r.m01 + b.radius.y * abs_r.m00;
		penetration = ra + rb - abs(t.z * r.m12 - t.y * r.m22);
		if (penetration < 0.f)
			return false;

		// Test a.y x b.x
		ra = a.radius.x * abs_r.m20 + a.radius.z * abs_r.m00;
		rb = b.radius.y * abs_r.m12 + b.radius.z * abs_r.m11;
		penetration = ra + rb - abs(t.x * r.m20 - t.z * r.m00);
		if (penetration < 0.f)
			return false;

		// Test a.y x b.y.
		ra = a.radius.x * abs_r.m21 + a.radius.z * abs_r.m01;
		rb = b.radius.x * abs_r.m12 + b.radius.z * abs_r.m10;
		penetration = ra + rb - abs(t.x * r.m21 - t.z * r.m01);
		if (penetration < 0.f)
			return false;

		// Test a.y x b.z
		ra = a.radius.x * abs_r.m22 + a.radius.z * abs_r.m02;
		rb = b.radius.x * abs_r.m11 + b.radius.y * abs_r.m10;
		penetration = ra + rb - abs(t.x * r.m22 - t.z * r.m02);
		if (penetration < 0.f)
			return false;

		// Test a.z x b.x
		ra = a.radius.x * abs_r.m10 + a.radius.y * abs_r.m00;
		rb = b.radius.y * abs_r.m22 + b.radius.z * abs_r.m21;
		penetration = ra + rb - abs(t.y * r.m00 - t.x * r.m10);
		if (penetration < 0.f)
			return false;

		// Test a.z x b.y
		ra = a.radius.x * abs_r.m11 + a.radius.y * abs_r.m01;
		rb = b.radius.x * abs_r.m22 + b.radius.z * abs_r.m20;
		penetration = ra + rb - abs(t.y * r.m01 - t.x * r.m11);
		if (penetration < 0.f)
			return false;

		// Test a.z x b.z
		ra = a.radius.x * abs_r.m12 + a.radius.y * abs_r.m02;
		rb = b.radius.x * abs_r.m21 + b.radius.y * abs_r.m20;
		penetration = ra + rb - abs(t.y * r.m02 - t.x * r.m12);
		if (penetration < 0.f)
			return false;

		return true;
	}

	bool obb_vs_hull(const BoundingOrientedBox& o, const BoundingHull& h)
	{
		OBBSupportFn obb_support{ o };
		HullSupportFn hull_support{ h };

		GJKSimplex gjk_simplex;
		return gjk_intersection_test(obb_support, hull_support, gjk_simplex);
	}

	bool obb_vs_plane(const BoundingOrientedBox& a, vec4 plane)
	{
		vec3 center = a.center;
		vec3 radius = a.radius;

		vec3 e0 = a.rotation * vec3(1.f, 0.f, 0.f);
		vec3 e1 = a.rotation * vec3(0.f, 1.f, 0.f);
		vec3 e2 = a.rotation * vec3(0.f, 0.f, 1.f);

		float r = radius.x * abs(dot(plane.xyz, e0))
			+ radius.y * abs(dot(plane.xyz, e1))
			+ radius.z * abs(dot(plane.xyz, e2));

		float s = abs(signed_distance_to_plane(center, plane));

		return s <= r;
	}

	bool obb_vs_triangle(const BoundingOrientedBox& obb, vec3 a, vec3 b, vec3 c)
	{
		a = conjugate(obb.rotation) * (a - obb.center);
		b = conjugate(obb.rotation) * (b - obb.center);
		c = conjugate(obb.rotation) * (c - obb.center);

		return aabb_vs_triangle({ -obb.radius, obb.radius }, a, b, c);
	}

	bool hull_vs_hull(const BoundingHull& a, const BoundingHull& b)
	{
		HullSupportFn hull_support1{ a };
		HullSupportFn hull_support2{ b };

		GJKSimplex gjk_simplex;
		return gjk_intersection_test(hull_support1, hull_support2, gjk_simplex);
	}

	float closest_point_segment_segment(const LineSegment& l1, const LineSegment& l2, vec3& c1, vec3& c2)
	{
		float s, t;
		vec3 d1 = l1.b - l1.a; // Direction vector of segment S1
		vec3 d2 = l2.b - l2.a; // Direction vector of segment S2
		vec3 r = l1.a - l2.a;
		float a = dot(d1, d1); // Squared length of segment S1, always nonnegative
		float e = dot(d2, d2); // Squared length of segment S2, always nonnegative
		float f = dot(d2, r);
		// Check if either or both segments degenerate into points
		if (a <= EPSILON && e <= EPSILON)
		{
			// Both segments degenerate into points
			s = t = 0.0f;
			c1 = l1.a;
			c2 = l2.a;
			return dot(c1 - c2, c1 - c2);
		}
		if (a <= EPSILON)
		{
			// First segment degenerates into a point
			s = 0.0f;
			t = f / e; // s = 0 => t = (b*s + f) / e = f / e
			t = clamp(t, 0.f, 1.f);
		}
		else
		{
			float c = dot(d1, r);
			if (e <= EPSILON)
			{
				// Second segment degenerates into a point
				t = 0.0f;
				s = clamp(-c / a, 0.f, 1.f); // t = 0 => s = (b*t - c) / a = -c / a
			}
			else
			{
				// The general nondegenerate case starts here
				float b = dot(d1, d2);
				float denom = a * e - b * b; // Always nonnegative
				// If segments not parallel, compute closest point on L1 to L2 and
				// clamp to segment S1. Else pick arbitrary s (here 0)
				if (denom != 0.f)
					s = clamp((b * f - c * e) / denom, 0.f, 1.f);
				else
					s = 0.0f;
				// Compute point on L2 closest to S1(s) using
				// t = dot((l1.a + D1*s) - l2.a,D2) / dot(D2,D2) = (b*s + f) / e
				t = (b * s + f) / e;
				// If t in [0,1] done. Else clamp t, recompute s for the new value
				// of t using s = dot((l2.a + D2*t) - l1.a,D1) / dot(D1,D1)= (t*b - c) / a
				// and clamp s to [0, 1]
				if (t < 0.f)
				{
					t = 0.f;
					s = clamp(-c / a, 0.f, 1.f);
				}
				else if (t > 1.0f)
				{
					t = 1.f;
					s = clamp((b - c) / a, 0.f, 1.f);
				}
			}
		}
		c1 = l1.a + d1 * s;
		c2 = l2.a + d2 * t;
		return squared_length(c1 - c2);
	}

	vec3 closest_point_point_triangle(const vec3& p, const vec3& a, const vec3& b, const vec3& c)
	{
		vec3 ab = b - a;
		vec3 ac = c - a;
		vec3 ap = p - a;
		float d1 = dot(ab, ap);
		float d2 = dot(ac, ap);
		if (d1 <= 0.f && d2 <= 0.f)
			return a; // Barycentric coordinates (1,0,0)

		// Check if P in vertex region outside B
		vec3 bp = p - b;
		float d3 = dot(ab, bp);
		float d4 = dot(ac, bp);
		if (d3 >= 0.f && d4 <= d3)
			return b; // Barycentric coordinates (0,1,0)

		// Check if P in edge region of AB, if so return projection of P onto AB
		float vc = d1 * d4 - d3 * d2;
		if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f)
		{
			float v = d1 / (d1 - d3);
			return a + v * ab; // Barycentric coordinates (1-v,v,0)
		}

		// Check if P in vertex region outside C
		vec3 cp = p - c;
		float d5 = dot(ab, cp);
		float d6 = dot(ac, cp);
		if (d6 >= 0.f && d5 <= d6)
			return c; // Barycentric coordinates (0,0,1)

		// Check if P in edge region of AC, if so return projection of P onto AC
		float vb = d5 * d2 - d1 * d6;
		if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f)
		{
			float w = d2 / (d2 - d6);
			return a + w * ac; // Barycentric coordinates (1-w,0,w)
		}

		// Check if P in edge region of BC, if so return projection of P onto BC
		float va = d3 * d6 - d5 * d4;
		if (va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f)
		{
			float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
			return b + w * (c - b); // Barycentric coordinates (0,1-w,w)
		}

		// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
		float denom = 1.f / (va + vb + vc);
		float v = vb * denom;
		float w = vc * denom;
		return a + ab * v + ac * w; // = u*a + v*b + w*c, u = va * denom = 1.0f - v - w
	}

	static void add_bounding_hull_edge(uint32 a, uint32 b, uint32 face, std::unordered_map<uint32, BoundingHullEdge*>& edge_map, std::vector<BoundingHullEdge>& edges, uint32& edge_index)
	{
		uint32 from = min(a, b);
		uint32 to = max(a, b);
		uint32 edge = (from << 16) | to;

		auto it = edge_map.find(edge);
		if (it == edge_map.end())
		{
			auto& new_edge = edges[edge_index++];
			new_edge.from = from;
			new_edge.to = to;
			new_edge.face_a = face;
			edge_map.insert({ edge, &new_edge });
		}
		else
		{
			ASSERT(it->second->from == from);
			ASSERT(it->second->to == to);
			ASSERT(it->second->face_a != UINT16_MAX);
			ASSERT(it->second->face_b == UINT16_MAX);
			it->second->face_b = face;
		}
	}

	template <typename triangle_t>
	static BoundingHullGeometry hull_from_mesh(vec3* vertices, uint32 num_vertices, triangle_t* triangles, uint32 num_triangles)
	{
		BoundingHullGeometry hull;

		uint32 num_edges = num_vertices + num_triangles - 2; // Euler characteristic for convex polyhedra

		hull.vertices.resize(num_vertices);
		hull.faces.resize(num_triangles);
		hull.edges.resize(num_edges, { UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX });
		hull.aabb = BoundingBox::negative_infinity();

		for (uint32 i = 0; i < num_vertices; ++i)
		{
			hull.vertices[i] = vertices[i];
			hull.aabb.grow(vertices[i]);
		}

		std::unordered_map<uint32, BoundingHullEdge*> edge_map;

		uint32 edge_index = 0;

		for (uint32 i = 0; i < num_triangles; ++i)
		{
			triangle_t tri = triangles[i];

			uint32 a = tri.a;
			uint32 b = tri.b;
			uint32 c = tri.c;

			vec3 va = vertices[a];
			vec3 vb = vertices[b];
			vec3 vc = vertices[c];
			vec3 normal = cross(vb - va, vc - va);

			hull.faces[i].a = a;
			hull.faces[i].b = b;
			hull.faces[i].c = c;
			hull.faces[i].normal = normal;

			add_bounding_hull_edge(a, b, i, edge_map, hull.edges, edge_index);
			add_bounding_hull_edge(b, c, i, edge_map, hull.edges, edge_index);
			add_bounding_hull_edge(a, c, i, edge_map, hull.edges, edge_index);
		}

		ASSERT(edge_index == num_edges);

		return hull;
	}

	BoundingHullGeometry BoundingHullGeometry::from_mesh(vec3* vertices, uint32 num_vertices, IndexedTriangle16* triangles, uint32 num_triangles)
	{
		return hull_from_mesh(vertices, num_vertices, triangles, num_triangles);
	}

	BoundingHullGeometry BoundingHullGeometry::from_mesh(vec3* vertices, uint32 num_vertices, IndexedTriangle32* triangles, uint32 num_triangles)
	{
		return hull_from_mesh(vertices, num_vertices, triangles, num_triangles);
	}

}