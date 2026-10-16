// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/bounding_volumes.h"

namespace era_engine
{
	struct ERA_CORE_API SphereSupportFn
	{
		vec3 operator()(vec3 dir) const
		{
			return normalize(dir) * s.radius + s.center;
		}

		const BoundingSphere& s;
	};

	struct ERA_CORE_API CapsuleSupportFn
	{
		vec3 operator()(const vec3& dir) const
		{
			float dist_a = dot(dir, c.position_a);
			float dist_b = dot(dir, c.position_b);
			vec3 farther_point = dist_a > dist_b ? c.position_a : c.position_b;
			return normalize(dir) * c.radius + farther_point;
		}

		const BoundingCapsule& c;
	};

	struct ERA_CORE_API CylinderSupportFn
	{
		vec3 operator()(const vec3& dir) const
		{
			float dist_a = dot(dir, c.position_a);
			float dist_b = dot(dir, c.position_b);
			vec3 farther_point = dist_a > dist_b ? c.position_a : c.position_b;

			vec3 n = c.position_a - c.position_b;

			vec3 projected_dir = noz(cross(cross(n, dir), n));
			return farther_point + projected_dir * c.radius;
		}

		const BoundingCylinder& c;
	};

	struct ERA_CORE_API AABBSupportFn
	{
		vec3 operator()(vec3 dir) const
		{
			return vec3(
				(dir.x < 0.f) ? b.min_corner.x : b.max_corner.x,
				(dir.y < 0.f) ? b.min_corner.y : b.max_corner.y,
				(dir.z < 0.f) ? b.min_corner.z : b.max_corner.z
			);
		}

		const BoundingBox& b;
	};

	struct ERA_CORE_API OBBSupportFn
	{
		vec3 operator()(vec3 dir) const
		{
			dir = conjugate(b.rotation) * dir;
			vec3 r(
				dir.x < 0.f ? -b.radius.x : b.radius.x,
				dir.y < 0.f ? -b.radius.y : b.radius.y,
				dir.z < 0.f ? -b.radius.z : b.radius.z
			);

			return b.center + b.rotation * r;
		}

		const BoundingOrientedBox& b;
	};

	struct ERA_CORE_API HullSupportFn
	{
		vec3 operator()(vec3 dir) const
		{
			dir = conjugate(h.rotation) * dir;
			vec3 result;

			float max_dist = -FLT_MAX;

			for (const vec3& v : h.geometry_ptr->vertices)
			{
				float d = dot(dir, v);
				if (d > max_dist)
				{
					max_dist = d;
					result = v;
				}
			}

			return h.position + h.rotation * result;
		};

		const BoundingHull& h;
	};

	union ExtrudedTriangleSupportFn
	{
		struct
		{
			vec3 a, b, c;
			vec3 d, e, f;
		};

		ExtrudedTriangleSupportFn(vec3 a, vec3 b, vec3 c, float extrusion = 10.f)
			: a(a), b(b), c(c),
			d(a.x, a.y - extrusion, a.z),
			e(b.x, b.y - extrusion, b.z),
			f(c.x, c.y - extrusion, c.z)
		{}

		vec3 operator()(vec3 dir) const
		{
			float max_d = dot(a, dir);
			vec3 result = a;

			for (uint32 i = 1; i < 6; ++i)
			{
				float d = dot(points[i], dir);
				if (d > max_d)
				{
					max_d = d;
					result = points[i];
				}
			}

			return result;
		}

		vec3 points[6];
	};

	struct ERA_CORE_API GJKSupportPoint
	{
		GJKSupportPoint()
		{
		}

		GJKSupportPoint(const vec3& a, const vec3& b)
		{
			shape_a_point = a;
			shape_b_point = b;
			minkowski = a - b;
		}

		vec3 shape_a_point;
		vec3 shape_b_point;
		vec3 minkowski;
	};

	struct ERA_CORE_API GJKSimplex
	{
		GJKSupportPoint a; // This is only used for the epa.
		GJKSupportPoint b, c, d;
		uint32 num_points = 0;
	};

	template <typename shapeA_t, typename shapeB_t>
	static GJKSupportPoint support(const shapeA_t& a, const shapeB_t& b, const vec3& dir)
	{
		vec3 farthest_in_a = a(dir);
		vec3 farthest_in_b = b(-dir);
		return GJKSupportPoint(farthest_in_a, farthest_in_b);
	}

	static inline vec3 cross_aba(const vec3& a, const vec3& b)
	{
		return cross(cross(a, b), a);
	}

	enum GJKInternalSuccess
	{
		GJK_STOP,
		GJK_DONT_STOP,
		GJK_UNEXPECTED_ERROR,
	};

	GJKInternalSuccess update_gjk_simplex(GJKSimplex& s, const GJKSupportPoint& a, vec3& dir);

	template <typename shapeA_t, typename shapeB_t>
	static bool gjk_intersection_test(const shapeA_t& shape_a, const shapeB_t& shape_b, GJKSimplex& out_simplex)
	{
		vec3 dir(1.f, 0.1f, -0.2f);

		// First point.
		out_simplex.c = support(shape_a, shape_b, dir);
		if (dot(out_simplex.c.minkowski, dir) < 0.f)
		{
			return false;
		}

		// Second point.
		dir = -out_simplex.c.minkowski;
		out_simplex.b = support(shape_a, shape_b, dir);
		if (dot(out_simplex.b.minkowski, dir) < 0.f)
		{
			return false;
		}

		dir = cross_aba(out_simplex.c.minkowski - out_simplex.b.minkowski, -out_simplex.b.minkowski);
		out_simplex.num_points = 2;

		while (true)
		{
			if (squared_length(dir) < 0.0001f)
			{
				return false;
			}

			GJKSupportPoint a = support(shape_a, shape_b, dir);
			if (dot(a.minkowski, dir) < 0.f)
			{
				return false;
			}

			GJKInternalSuccess success = update_gjk_simplex(out_simplex, a, dir);
			if (success == GJK_STOP)
			{
				ASSERT(out_simplex.num_points == 3);
				out_simplex.a = a;
				out_simplex.num_points = 4;
				break;
			}
			else if (success == GJK_UNEXPECTED_ERROR)
			{
				return false;
			}
		}

		return true;
	}
}