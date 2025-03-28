// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/bounding_volumes.h"

namespace era_engine
{
	struct ERA_CORE_API sphere_support_fn
	{
		vec3 operator()(vec3 dir) const
		{
			return normalize(dir) * s.radius + s.center;
		}

		const bounding_sphere& s;
	};

	struct ERA_CORE_API capsule_support_fn
	{
		vec3 operator()(const vec3& dir) const
		{
			float distA = dot(dir, c.positionA);
			float distB = dot(dir, c.positionB);
			vec3 fartherPoint = distA > distB ? c.positionA : c.positionB;
			return normalize(dir) * c.radius + fartherPoint;
		}

		const bounding_capsule& c;
	};

	struct ERA_CORE_API cylinder_support_fn
	{
		vec3 operator()(const vec3& dir) const
		{
			float distA = dot(dir, c.positionA);
			float distB = dot(dir, c.positionB);
			vec3 fartherPoint = distA > distB ? c.positionA : c.positionB;

			vec3 n = c.positionA - c.positionB;

			vec3 projectedDir = noz(cross(cross(n, dir), n));
			return fartherPoint + projectedDir * c.radius;
		}

		const bounding_cylinder& c;
	};

	struct ERA_CORE_API aabb_support_fn
	{
		vec3 operator()(vec3 dir) const
		{
			return vec3(
				(dir.x < 0.f) ? b.minCorner.x : b.maxCorner.x,
				(dir.y < 0.f) ? b.minCorner.y : b.maxCorner.y,
				(dir.z < 0.f) ? b.minCorner.z : b.maxCorner.z
			);
		}

		const bounding_box& b;
	};

	struct ERA_CORE_API obb_support_fn
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

		const bounding_oriented_box& b;
	};

	struct ERA_CORE_API hull_support_fn
	{
		vec3 operator()(vec3 dir) const
		{
			dir = conjugate(h.rotation) * dir;
			vec3 result;

			float maxDist = -FLT_MAX;

			for (const vec3& v : h.geometryPtr->vertices)
			{
				float d = dot(dir, v);
				if (d > maxDist)
				{
					maxDist = d;
					result = v;
				}
			}

			return h.position + h.rotation * result;
		};

		const bounding_hull& h;
	};

	union extruded_triangle_support_fn
	{
		struct
		{
			vec3 a, b, c;
			vec3 d, e, f;
		};

		extruded_triangle_support_fn(vec3 a, vec3 b, vec3 c, float extrusion = 10.f)
			: a(a), b(b), c(c),
			d(a.x, a.y - extrusion, a.z),
			e(b.x, b.y - extrusion, b.z),
			f(c.x, c.y - extrusion, c.z)
		{}

		vec3 operator()(vec3 dir) const
		{
			float maxD = dot(a, dir);
			vec3 result = a;

			for (uint32 i = 1; i < 6; ++i)
			{
				float d = dot(points[i], dir);
				if (d > maxD)
				{
					maxD = d;
					result = points[i];
				}
			}

			return result;
		}

		vec3 points[6];
	};

	struct ERA_CORE_API gjk_support_point
	{
		gjk_support_point()
		{
		}

		gjk_support_point(const vec3& a, const vec3& b)
		{
			shapeAPoint = a;
			shapeBPoint = b;
			minkowski = a - b;
		}

		vec3 shapeAPoint;
		vec3 shapeBPoint;
		vec3 minkowski;
	};

	struct ERA_CORE_API gjk_simplex
	{
		gjk_support_point a; // This is only used for the epa.
		gjk_support_point b, c, d;
		uint32 numPoints = 0;
	};

	template <typename shapeA_t, typename shapeB_t>
	static gjk_support_point support(const shapeA_t& a, const shapeB_t& b, const vec3& dir)
	{
		vec3 farthestInA = a(dir);
		vec3 farthestInB = b(-dir);
		return gjk_support_point(farthestInA, farthestInB);
	}

	static inline vec3 crossABA(const vec3& a, const vec3& b)
	{
		return cross(cross(a, b), a);
	}

	enum gjk_internal_success
	{
		gjk_stop,
		gjk_dont_stop,
		gjk_unexpected_error, // Happens very very seldom. I don't know the reason yet, but instead of asserting we return this. I figure it's better than crashing.
	};

	gjk_internal_success updateGJKSimplex(gjk_simplex& s, const gjk_support_point& a, vec3& dir);

	template <typename shapeA_t, typename shapeB_t>
	static bool gjkIntersectionTest(const shapeA_t& shapeA, const shapeB_t& shapeB, gjk_simplex& outSimplex)
	{
		vec3 dir(1.f, 0.1f, -0.2f); // Arbitrary.

		// First point.
		outSimplex.c = support(shapeA, shapeB, dir);
		if (dot(outSimplex.c.minkowski, dir) < 0.f)
			return false;

		// Second point.
		dir = -outSimplex.c.minkowski;
		outSimplex.b = support(shapeA, shapeB, dir);
		if (dot(outSimplex.b.minkowski, dir) < 0.f)
			return false;

		dir = crossABA(outSimplex.c.minkowski - outSimplex.b.minkowski, -outSimplex.b.minkowski);
		outSimplex.numPoints = 2;

		while (true)
		{
			if (squared_length(dir) < 0.0001f)
				return false;

			gjk_support_point a = support(shapeA, shapeB, dir);
			if (dot(a.minkowski, dir) < 0.f)
				return false;

			gjk_internal_success success = updateGJKSimplex(outSimplex, a, dir);
			if (success == gjk_stop)
			{
				ASSERT(outSimplex.numPoints == 3);
				outSimplex.a = a;
				outSimplex.numPoints = 4;
				break;
			}
			else if (success == gjk_unexpected_error)
				return false;
		}

		return true;
	}
}