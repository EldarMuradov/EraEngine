// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/bounding_volumes.h"
#include "core/collision_gjk.h"

namespace era_engine
{
	static bool pointInAABB(vec3 point, bounding_box aabb)
	{
		return point.x >= aabb.minCorner.x
			&& point.y >= aabb.minCorner.y
			&& point.z >= aabb.minCorner.z
			&& point.x <= aabb.maxCorner.x
			&& point.y <= aabb.maxCorner.y
			&& point.z <= aabb.maxCorner.z;
	}

	static float closestPointOnLineSegment(vec3 point, vec3 lineA, vec3 lineB)
	{
		vec3 ab = lineB - lineA;
		float t = dot(point - lineA, ab) / squared_length(ab);
		t = clamp(t, 0.f, 1.f);
		return t;
	}

	void bounding_box::grow(vec3 o)
	{
		minCorner.x = min(minCorner.x, o.x);
		minCorner.y = min(minCorner.y, o.y);
		minCorner.z = min(minCorner.z, o.z);
		maxCorner.x = max(maxCorner.x, o.x);
		maxCorner.y = max(maxCorner.y, o.y);
		maxCorner.z = max(maxCorner.z, o.z);
	}

	void bounding_box::pad(vec3 p)
	{
		minCorner -= p;
		maxCorner += p;
	}

	vec3 bounding_box::getCenter() const
	{
		return (minCorner + maxCorner) * 0.5f;
	}

	vec3 bounding_box::getRadius() const
	{
		return (maxCorner - minCorner) * 0.5f;
	}

	bool bounding_box::contains(vec3 p) const
	{
		return p.x >= minCorner.x && p.x <= maxCorner.x
			&& p.y >= minCorner.y && p.y <= maxCorner.y
			&& p.z >= minCorner.z && p.z <= maxCorner.z;
	}

	bounding_box bounding_box::transformToAABB(quat rotation, vec3 translation) const
	{
		bounding_box result = bounding_box::negativeInfinity();
		result.grow(rotation * minCorner + translation);
		result.grow(rotation * vec3(maxCorner.x, minCorner.y, minCorner.z) + translation);
		result.grow(rotation * vec3(minCorner.x, maxCorner.y, minCorner.z) + translation);
		result.grow(rotation * vec3(maxCorner.x, maxCorner.y, minCorner.z) + translation);
		result.grow(rotation * vec3(minCorner.x, minCorner.y, maxCorner.z) + translation);
		result.grow(rotation * vec3(maxCorner.x, minCorner.y, maxCorner.z) + translation);
		result.grow(rotation * vec3(minCorner.x, maxCorner.y, maxCorner.z) + translation);
		result.grow(rotation * maxCorner + translation);
		return result;
	}

	bounding_oriented_box bounding_box::transformToOBB(quat rotation, vec3 translation) const
	{
		bounding_oriented_box obb;
		obb.center = rotation * getCenter() + translation;
		obb.radius = getRadius();
		obb.rotation = rotation;
		return obb;
	}

	bounding_box_corners bounding_box::getCorners() const
	{
		bounding_box_corners result;
		result.i = minCorner;
		result.x = vec3(maxCorner.x, minCorner.y, minCorner.z);
		result.y = vec3(minCorner.x, maxCorner.y, minCorner.z);
		result.xy = vec3(maxCorner.x, maxCorner.y, minCorner.z);
		result.z = vec3(minCorner.x, minCorner.y, maxCorner.z);
		result.xz = vec3(maxCorner.x, minCorner.y, maxCorner.z);
		result.yz = vec3(minCorner.x, maxCorner.y, maxCorner.z);
		result.xyz = maxCorner;
		return result;
	}

	bounding_box_corners bounding_box::getCorners(quat rotation, vec3 translation) const
	{
		bounding_box_corners result;
		result.i = rotation * minCorner + translation;
		result.x = rotation * vec3(maxCorner.x, minCorner.y, minCorner.z) + translation;
		result.y = rotation * vec3(minCorner.x, maxCorner.y, minCorner.z) + translation;
		result.xy = rotation * vec3(maxCorner.x, maxCorner.y, minCorner.z) + translation;
		result.z = rotation * vec3(minCorner.x, minCorner.y, maxCorner.z) + translation;
		result.xz = rotation * vec3(maxCorner.x, minCorner.y, maxCorner.z) + translation;
		result.yz = rotation * vec3(minCorner.x, maxCorner.y, maxCorner.z) + translation;
		result.xyz = rotation * maxCorner + translation;
		return result;
	}

	bounding_box bounding_box::everything()
	{
		return bounding_box{ vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX), vec3(FLT_MAX, FLT_MAX, FLT_MAX) };
	}

	bounding_box bounding_box::negativeInfinity()
	{
		return bounding_box{ vec3(FLT_MAX, FLT_MAX, FLT_MAX), vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX) };
	}

	bounding_box bounding_box::fromMinMax(vec3 minCorner, vec3 maxCorner)
	{
		return bounding_box{ minCorner, maxCorner };
	}

	bounding_box bounding_box::fromCenterRadius(vec3 center, vec3 radius)
	{
		return bounding_box{ center - radius, center + radius };
	}

	bounding_box bounding_oriented_box::getAABB() const
	{
		bounding_box bb = { -radius, radius };
		return bb.transformToAABB(rotation, center);
	}

	bounding_box bounding_oriented_box::transformToAABB(quat rotation, vec3 translation) const
	{
		bounding_box bb = { -radius, radius };
		return bb.transformToAABB(rotation * this->rotation, rotation * center + translation);
	}

	bounding_oriented_box bounding_oriented_box::transformToOBB(quat rotation, vec3 translation) const
	{
		return bounding_oriented_box{ rotation * this->rotation, rotation * center + translation, radius };
	}

	bounding_box_corners bounding_oriented_box::getCorners() const
	{
		bounding_box bb = { -radius, radius };
		return bb.getCorners(rotation, center);
	}

	void bounding_rectangle::grow(vec2 o)
	{
		minCorner.x = min(minCorner.x, o.x);
		minCorner.y = min(minCorner.y, o.y);
		maxCorner.x = max(maxCorner.x, o.x);
		maxCorner.y = max(maxCorner.y, o.y);
	}

	void bounding_rectangle::pad(vec2 p)
	{
		minCorner -= p;
		maxCorner += p;
	}

	vec2 bounding_rectangle::getCenter() const
	{
		return (minCorner + maxCorner) * 0.5f;
	}

	vec2 bounding_rectangle::getRadius() const
	{
		return (maxCorner - minCorner) * 0.5f;
	}

	bool bounding_rectangle::contains(vec2 p) const
	{
		return p.x >= minCorner.x && p.x <= maxCorner.x
			&& p.y >= minCorner.y && p.y <= maxCorner.y;
	}

	bounding_rectangle bounding_rectangle::negativeInfinity()
	{
		return bounding_rectangle{ vec2(FLT_MAX, FLT_MAX), vec2(-FLT_MAX, -FLT_MAX) };
	}

	bounding_rectangle bounding_rectangle::fromMinMax(vec2 minCorner, vec2 maxCorner)
	{
		return bounding_rectangle{ minCorner, maxCorner };
	}

	bounding_rectangle bounding_rectangle::fromCenterRadius(vec2 center, vec2 radius)
	{
		return bounding_rectangle{ center - radius, center + radius };
	}

	bool ray::intersectPlane(vec3 normal, float d, float& outT) const
	{
		float ndotd = dot(direction, normal);
		if (abs(ndotd) < 1e-6f)
		{
			return false;
		}

		outT = -(dot(origin, normal) + d) / ndotd;
		return true;
	}

	bool ray::intersectPlane(vec3 normal, vec3 point, float& outT) const
	{
		float d = -dot(normal, point);
		return intersectPlane(normal, d, outT);
	}

	bool ray::intersectAABB(const bounding_box& a, float& outT) const
	{
		vec3 invDir = vec3(1.f / direction.x, 1.f / direction.y, 1.f / direction.z); // This can be Inf (when one direction component is 0) but still works

		float tx1 = (a.minCorner.x - origin.x) * invDir.x;
		float tx2 = (a.maxCorner.x - origin.x) * invDir.x;

		outT = min(tx1, tx2);
		float tmax = max(tx1, tx2);

		float ty1 = (a.minCorner.y - origin.y) * invDir.y;
		float ty2 = (a.maxCorner.y - origin.y) * invDir.y;

		outT = max(outT, min(ty1, ty2));
		tmax = min(tmax, max(ty1, ty2));

		float tz1 = (a.minCorner.z - origin.z) * invDir.z;
		float tz2 = (a.maxCorner.z - origin.z) * invDir.z;

		outT = max(outT, min(tz1, tz2));
		tmax = min(tmax, max(tz1, tz2));

		bool result = tmax >= outT && outT > 0.f;

		return result;
	}

	bool ray::intersectOBB(const bounding_oriented_box& a, float& outT) const
	{
		//return (conjugate(m.rotation) * (pos - m.position)) / m.scale;
		ray localR = { conjugate(a.rotation) * (origin - a.center), conjugate(a.rotation) * direction };
		return localR.intersectAABB(bounding_box::fromCenterRadius(0.f, a.radius), outT);
	}

	bool ray::intersectTriangle(vec3 a, vec3 b, vec3 c, float& outT, bool& outFrontFacing) const
	{
		vec3 normal = noz(cross(b - a, c - a));
		float d = -dot(normal, a);

		float nDotR = dot(direction, normal);
		if (fabsf(nDotR) <= 1e-6f)
			return false;

		outT = -(dot(origin, normal) + d) / nDotR;

		vec3 q = origin + outT * direction;
		outFrontFacing = nDotR < 0.f;
		return outT >= 0.f && point_in_triangle(q, a, b, c);
	}

	bool ray::intersectSphere(vec3 center, float radius, float& outT) const
	{
		vec3 m = origin - center;
		float b = dot(m, direction);
		float c = dot(m, m) - radius * radius;

		if (c > 0.f && b > 0.f)
			return false;

		float discr = b * b - c;

		if (discr < 0.f)
			return false;

		outT = -b - sqrt(discr);

		if (outT < 0.f)
			outT = 0.f;

		return true;
	}

	bool ray::intersectCylinder(const bounding_cylinder& cylinder, float& outT) const
	{
		vec3 d = direction;
		vec3 o = origin;

		vec3 axis = cylinder.positionB - cylinder.positionA;
		float height = length(axis);

		quat q = rotate_from_to(axis, vec3(0.f, 1.f, 0.f));

		// Cylinder base is at 0,0,0. Second point is above it
		o = q * (o - cylinder.positionA);
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

			outT = (-b - sqrt(delta)) / a;
			if (outT <= epsilon)
				return false; // Behind ray

			y = o.y + outT * d.y;
		}

		// Check bases. Always true if the above if is not taken
		if (y > height + epsilon || y < -epsilon)
		{
			vec3 posA = vec3(0.f, 0.f, 0.f);
			vec3 posB = vec3(0.f, height, 0.f);

			ray localRay = { o, d };

			float dist;
			bool b1 = d.y < 0.f && localRay.intersectDisk(posB, vec3(0.f, 1.f, 0.f), cylinder.radius, dist);
			if (b1)
				outT = dist;
			bool b2 = d.y > 0.f && localRay.intersectDisk(posA, vec3(0.f, -1.f, 0.f), cylinder.radius, dist);
			if (b2)
				outT = dist;

			y = o.y + outT * d.y;
		}

		return y > -epsilon && y < height + epsilon;
	}

	bool ray::intersectCapsule(const bounding_capsule& capsule, float& outT) const
	{
		outT = FLT_MAX;
		float t;
		bool result = false;
		if (intersectCylinder(bounding_cylinder{ capsule.positionA, capsule.positionB, capsule.radius }, t))
		{
			outT = t;
			result = true;
		}
		if (intersectSphere(bounding_sphere{ capsule.positionA, capsule.radius }, t))
		{
			outT = min(outT, t);
			result = true;
		}
		if (intersectSphere(bounding_sphere{ capsule.positionB, capsule.radius }, t))
		{
			outT = min(outT, t);
			result = true;
		}
		return result;
	}

	bool ray::intersectDisk(vec3 pos, vec3 normal, float radius, float& outT) const
	{
		bool intersectsPlane = intersectPlane(normal, pos, outT);
		if (intersectsPlane)
			return length(origin + outT * direction - pos) <= radius;
		return false;
	}

	bool ray::intersectRectangle(vec3 pos, vec3 tangent, vec3 bitangent, vec2 radius, float& outT) const
	{
		vec3 normal = cross(tangent, bitangent);
		bool intersectsPlane = intersectPlane(normal, pos, outT);
		if (intersectsPlane)
		{
			vec3 offset = origin + outT * direction - pos;
			vec2 projected(dot(offset, tangent), dot(offset, bitangent));
			projected = abs(projected);
			if (projected.x <= radius.x && projected.y <= radius.y)
				return true;
		}
		return false;
	}

	static bool isZero(float x)
	{
		return abs(x) < 1e-6f;
	}

	struct solve_2_result
	{
		uint32 numResults;
		float results[2];
	};

	struct solve_3_result
	{
		uint32 numResults;
		float results[3];
	};

	struct solve_4_result
	{
		uint32 numResults;
		float results[4];
	};

	static solve_2_result solve2(float c0, float c1, float c2)
	{
		float p = c1 / (2 * c2);
		float q = c0 / c2;

		float D = p * p - q;

		if (isZero(D))
			return { 1, -p };
		else if (D < 0)
			return { 0 };
		else /* if (D > 0) */
		{
			float sqrt_D = sqrt(D);

			return { 2, sqrt_D - p, -sqrt_D - p };
		}
	}

	static solve_3_result solve3(float c0, float c1, float c2, float c3)
	{
		float A = c2 / c3;
		float B = c1 / c3;
		float C = c0 / c3;

		float sq_A = A * A;
		float p = 1.f / 3 * (-1.f / 3 * sq_A + B);
		float q = 1.f / 2 * (2.f / 27 * A * sq_A - 1.f / 3 * A * B + C);

		/* use Cardano's formula */

		float cb_p = p * p * p;
		float D = q * q + cb_p;

		solve_3_result s = {};

		if (isZero(D))
		{
			if (isZero(q))
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
			float sqrt_D = sqrt(D);
			float u = cbrt(sqrt_D - q);
			float v = -cbrt(sqrt_D + q);

			s = { 1, u + v };
		}

		/* resubstitute */
		float sub = 1.f / 3.f * A;

		for (uint32 i = 0; i < s.numResults; ++i)
			s.results[i] -= sub;

		return s;
	}

	/**
	 *  Solves equation:
	 *
	 *      c[0] + c[1]*x + c[2]*x^2 + c[3]*x^3 + c[4]*x^4 = 0
	 *
	 */
	static solve_4_result solve4(float c0, float c1, float c2, float c3, float c4)
	{
		/* normal form: x^4 + Ax^3 + Bx^2 + Cx + D = 0 */

		float A = c3 / c4;
		float B = c2 / c4;
		float C = c1 / c4;
		float D = c0 / c4;

		/*  substitute x = y - A/4 to eliminate cubic term:
		x^4 + px^2 + qx + r = 0 */

		float sq_A = A * A;
		float p = -3.f / 8 * sq_A + B;
		float q = 1.f / 8 * sq_A * A - 1.f / 2 * A * B + C;
		float r = -3.f / 256 * sq_A * sq_A + 1.f / 16 * sq_A * B - 1.f / 4 * A * C + D;
		solve_4_result s = {};

		if (isZero(r))
		{
			/* no absolute term: y(y^3 + py + q) = 0 */

			auto s3 = solve3(q, p, 0, 1);
			for (uint32 i = 0; i < s3.numResults; ++i)
				s.results[s.numResults++] = s3.results[i];

			s.results[s.numResults++] = 0.f;
		}
		else
		{
			/* solve the resolvent cubic ... */
			auto s3 = solve3(1.f / 2 * r * p - 1.f / 8 * q * q, -r, -0.5f * p, 1.f);
			for (uint32 i = 0; i < s3.numResults; ++i)
				s.results[s.numResults++] = s3.results[i];

			/* ... and take the one real solution ... */
			float z = s.results[0];

			/* ... to build two quadric equations */
			float u = z * z - r;
			float v = 2.f * z - p;

			if (isZero(u))
				u = 0;
			else if (u > 0)
				u = sqrt(u);
			else
				return {};

			if (isZero(v))
				v = 0;
			else if (v > 0)
				v = sqrt(v);
			else
				return {};

			auto s2 = solve2(z - u, q < 0 ? -v : v, 1);
			s = {};
			for (uint32 i = 0; i < s2.numResults; ++i)
				s.results[s.numResults++] = s2.results[i];

			s2 = solve2(z + u, q < 0 ? v : -v, 1);
			for (uint32 i = 0; i < s2.numResults; ++i)
				s.results[s.numResults++] = s2.results[i];
		}

		/* resubstitute */
		float sub = 1.f / 4 * A;

		for (uint32 i = 0; i < s.numResults; ++i)
			s.results[i] -= sub;

		return s;
	}

	bool ray::intersectTorus(const bounding_torus& torus, float& outT) const
	{
		vec3 d = direction;
		vec3 o = origin;

		vec3 axis = torus.upAxis;

		quat q = rotate_from_to(axis, vec3(0.f, 1.f, 0.f));

		o = q * (o - torus.position);
		d = q * d;

		// define the coefficients of the quartic equation
		float sum_d_sqrd = dot(d, d);

		float e = dot(o, o) - torus.majorRadius * torus.majorRadius - torus.tubeRadius * torus.tubeRadius;
		float f = dot(o, d);
		float four_a_sqrd = 4.f * torus.majorRadius * torus.majorRadius;

		auto solution = solve4(
			e * e - four_a_sqrd * (torus.tubeRadius * torus.tubeRadius - o.y * o.y),
			4.f * f * e + 2.f * four_a_sqrd * o.y * d.y,
			2.f * sum_d_sqrd * e + 4.f * f * f + four_a_sqrd * d.y * d.y,
			4.f * sum_d_sqrd * f,
			sum_d_sqrd * sum_d_sqrd
		);

		// ray misses the torus
		if (solution.numResults == 0)
			return false;

		// find the smallest root greater than kEpsilon, if any
		// the roots array is not sorted
		float minT = FLT_MAX;
		for (uint32 i = 0; i < solution.numResults; ++i)
		{
			float t = solution.results[i];
			if ((t > 1e-6f) && (t < minT))
				minT = t;
		}
		outT = minT;

		return true;
	}

	bool ray::intersectHull(const bounding_hull& hull, const bounding_hull_geometry& geometry, float& outT) const
	{
		ray localR = { conjugate(hull.rotation) * (origin - hull.position), conjugate(hull.rotation) * direction };

		float minT = FLT_MAX;
		bool result = false;

		for (uint32 i = 0; i < (uint32)geometry.faces.size(); ++i)
		{
			auto tri = geometry.faces[i];
			vec3 a = geometry.vertices[tri.a];
			vec3 b = geometry.vertices[tri.b];
			vec3 c = geometry.vertices[tri.c];

			float t;
			bool ff;
			if (localR.intersectTriangle(a, b, c, t, ff) && t < minT)
			{
				minT = t;
				result = true;
			}
		}

		outT = minT;
		return result;
	}

	bool sphereVsCylinder(const bounding_sphere& s, const bounding_cylinder& c)
	{
		vec3 ab = c.positionB - c.positionA;
		float t = dot(s.center - c.positionA, ab) / squared_length(ab);
		if (t >= 0.f && t <= 1.f)
			return sphereVsSphere(s, bounding_sphere{ lerp(c.positionA, c.positionB, t), c.radius });

		vec3 p = (t <= 0.f) ? c.positionA : c.positionB;
		vec3 up = (t <= 0.f) ? -ab : ab;

		vec3 projectedDirToCenter = normalize(cross(cross(up, s.center - p), up));
		vec3 endA = p + projectedDirToCenter * c.radius;
		vec3 endB = p - projectedDirToCenter * c.radius;

		vec3 closestToSphere = closestPoint_PointSegment(s.center, line_segment{ endA, endB });
		float sqDistance = squared_length(closestToSphere - s.center);

		return sqDistance <= s.radius;
	}

	bool sphereVsHull(const bounding_sphere& s, const bounding_hull& h)
	{
		sphere_support_fn sphereSupport{ s };
		hull_support_fn hullSupport{ h };

		gjk_simplex gjkSimplex;
		return gjkIntersectionTest(sphereSupport, hullSupport, gjkSimplex);
	}

	bool sphereVsTriangle(const bounding_sphere& s, vec3 a, vec3 b, vec3 c)
	{
		vec3 p = closestPoint_PointTriangle(s.center, a, b, c);
		vec3 v = p - s.center;
		return dot(v, v) <= s.radius * s.radius;
	}

	bool capsuleVsAABB(const bounding_capsule& c, const bounding_box& b)
	{
		capsule_support_fn capsuleSupport{ c };
		aabb_support_fn boxSupport{ b };

		gjk_simplex gjkSimplex;
		return gjkIntersectionTest(capsuleSupport, boxSupport, gjkSimplex);
	}

	bool capsuleVsOBB(const bounding_capsule& c, const bounding_oriented_box& o)
	{
		bounding_box aabb = bounding_box::fromCenterRadius(o.center, o.radius);
		bounding_capsule c_ = {
			conjugate(o.rotation) * (c.positionA - o.center) + o.center,
			conjugate(o.rotation) * (c.positionB - o.center) + o.center,
			c.radius };

		return capsuleVsAABB(c_, aabb);
	}

	bool capsuleVsHull(const bounding_capsule& c, const bounding_hull& h)
	{
		capsule_support_fn capsuleSupport{ c };
		hull_support_fn hullSupport{ h };

		gjk_simplex gjkSimplex;
		return gjkIntersectionTest(capsuleSupport, hullSupport, gjkSimplex);
	}

	bool capsuleVsTriangle(const bounding_capsule& capsule, vec3 a, vec3 b, vec3 c)
	{
		ray r = { capsule.positionA, normalize(capsule.positionB - capsule.positionA) };

		vec3 triNormal = normalize(cross(b - a, c - a));
		float d = -dot(triNormal, a);

		float ndotd = dot(r.direction, triNormal);

		float t = -(dot(r.origin, triNormal) + d) / ndotd;

		vec3 trace = r.origin + t * r.direction;
		vec3 closest = closestPoint_PointTriangle(trace, a, b, c);

		vec3 reference = closestPoint_PointSegment(closest, { capsule.positionA, capsule.positionB });

		return sphereVsTriangle(bounding_sphere{ reference, capsule.radius }, a, b, c);
	}

	bool cylinderVsCylinder(const bounding_cylinder& a, const bounding_cylinder& b)
	{
		cylinder_support_fn cylinderSupportA{ a };
		cylinder_support_fn cylinderSupportB{ b };

		gjk_simplex gjkSimplex;
		return gjkIntersectionTest(cylinderSupportA, cylinderSupportB, gjkSimplex);
	}

	bool cylinderVsAABB(const bounding_cylinder& c, const bounding_box& b)
	{
		cylinder_support_fn cylinderSupport{ c };
		aabb_support_fn boxSupport{ b };

		gjk_simplex gjkSimplex;
		return gjkIntersectionTest(cylinderSupport, boxSupport, gjkSimplex);
	}

	bool cylinderVsOBB(const bounding_cylinder& c, const bounding_oriented_box& o)
	{
		bounding_box aabb = bounding_box::fromCenterRadius(o.center, o.radius);
		bounding_cylinder c_ = {
			conjugate(o.rotation) * (c.positionA - o.center) + o.center,
			conjugate(o.rotation) * (c.positionB - o.center) + o.center,
			c.radius };

		return cylinderVsAABB(c_, aabb);
	}

	bool cylinderVsHull(const bounding_cylinder& c, const bounding_hull& h)
	{
		cylinder_support_fn cylinderSupport{ c };
		hull_support_fn hullSupport{ h };

		gjk_simplex gjkSimplex;
		return gjkIntersectionTest(cylinderSupport, hullSupport, gjkSimplex);
	}

	bool aabbVsHull(const bounding_box& a, const bounding_hull& h)
	{
		aabb_support_fn aabbSupport{ a };
		hull_support_fn hullSupport{ h };

		gjk_simplex gjkSimplex;
		return gjkIntersectionTest(aabbSupport, hullSupport, gjkSimplex);
	}

	bool aabbVsPlane(const bounding_box& a, vec4 plane)
	{
		vec3 center = a.getCenter();
		vec3 radius = a.getRadius();

		float r = dot(radius, abs(plane.xyz));
		float s = abs(signedDistanceToPlane(center, plane));

		return s <= r;
	}

	bool aabbVsTriangle(const bounding_box& aabb, vec3 a, vec3 b, vec3 c)
	{
		vec3 radius = aabb.getRadius();
		vec3 center = aabb.getCenter();

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
			vec3 triNormal = cross(f0, f1);
			float triD = dot(triNormal, a);

			float r = dot(radius, abs(triNormal));
			float s = abs(triD);

			if (s > r)
				return false;
		}

		return true;
	}

	bool obbVsOBB(const bounding_oriented_box& a, const bounding_oriented_box& b)
	{
		union obb_axes
		{
			struct
			{
				vec3 x, y, z;
			};
			vec3 u[3];
		};

		float ra, rb, penetration;

		obb_axes axesA = {
			a.rotation * vec3(1.f, 0.f, 0.f),
			a.rotation * vec3(0.f, 1.f, 0.f),
			a.rotation * vec3(0.f, 0.f, 1.f),
		};

		obb_axes axesB = {
			b.rotation * vec3(1.f, 0.f, 0.f),
			b.rotation * vec3(0.f, 1.f, 0.f),
			b.rotation * vec3(0.f, 0.f, 1.f),
		};

		mat3 r;
		r.m00 = dot(axesA.x, axesB.x);
		r.m10 = dot(axesA.y, axesB.x);
		r.m20 = dot(axesA.z, axesB.x);
		r.m01 = dot(axesA.x, axesB.y);
		r.m11 = dot(axesA.y, axesB.y);
		r.m21 = dot(axesA.z, axesB.y);
		r.m02 = dot(axesA.x, axesB.z);
		r.m12 = dot(axesA.y, axesB.z);
		r.m22 = dot(axesA.z, axesB.z);

		vec3 tw = b.center - a.center;
		vec3 t = conjugate(a.rotation) * tw;

		mat3 absR;
		for (uint32 i = 0; i < 9; ++i)
			absR.m[i] = abs(r.m[i]) + EPSILON; // Add in an epsilon term to counteract arithmetic errors when two edges are parallel and their cross product is (near) 0

		// Test a's faces
		for (uint32 i = 0; i < 3; ++i)
		{
			ra = a.radius.data[i];
			rb = dot(row(absR, i), b.radius);
			float d = t.data[i];
			penetration = ra + rb - abs(d);
			if (penetration < 0.f)
				return false;
		}

		// Test b's faces
		for (uint32 i = 0; i < 3; ++i)
		{
			ra = dot(col(absR, i), a.radius);
			rb = b.radius.data[i];
			float d = dot(col(r, i), t);
			penetration = ra + rb - abs(d);
			if (penetration < 0.f)
				return false;
		}

		// Test a.x x b.x
		ra = a.radius.y * absR.m20 + a.radius.z * absR.m10;
		rb = b.radius.y * absR.m02 + b.radius.z * absR.m01;
		penetration = ra + rb - abs(t.z * r.m10 - t.y * r.m20);
		if (penetration < 0.f)
			return false;

		// Test a.x x b.y.
		ra = a.radius.y * absR.m21 + a.radius.z * absR.m11;
		rb = b.radius.x * absR.m02 + b.radius.z * absR.m00;
		penetration = ra + rb - abs(t.z * r.m11 - t.y * r.m21);
		if (penetration < 0.f)
			return false;

		// Test a.x x b.z
		ra = a.radius.y * absR.m22 + a.radius.z * absR.m12;
		rb = b.radius.x * absR.m01 + b.radius.y * absR.m00;
		penetration = ra + rb - abs(t.z * r.m12 - t.y * r.m22);
		if (penetration < 0.f)
			return false;

		// Test a.y x b.x
		ra = a.radius.x * absR.m20 + a.radius.z * absR.m00;
		rb = b.radius.y * absR.m12 + b.radius.z * absR.m11;
		penetration = ra + rb - abs(t.x * r.m20 - t.z * r.m00);
		if (penetration < 0.f)
			return false;

		// Test a.y x b.y.
		ra = a.radius.x * absR.m21 + a.radius.z * absR.m01;
		rb = b.radius.x * absR.m12 + b.radius.z * absR.m10;
		penetration = ra + rb - abs(t.x * r.m21 - t.z * r.m01);
		if (penetration < 0.f)
			return false;

		// Test a.y x b.z
		ra = a.radius.x * absR.m22 + a.radius.z * absR.m02;
		rb = b.radius.x * absR.m11 + b.radius.y * absR.m10;
		penetration = ra + rb - abs(t.x * r.m22 - t.z * r.m02);
		if (penetration < 0.f)
			return false;

		// Test a.z x b.x
		ra = a.radius.x * absR.m10 + a.radius.y * absR.m00;
		rb = b.radius.y * absR.m22 + b.radius.z * absR.m21;
		penetration = ra + rb - abs(t.y * r.m00 - t.x * r.m10);
		if (penetration < 0.f)
			return false;

		// Test a.z x b.y
		ra = a.radius.x * absR.m11 + a.radius.y * absR.m01;
		rb = b.radius.x * absR.m22 + b.radius.z * absR.m20;
		penetration = ra + rb - abs(t.y * r.m01 - t.x * r.m11);
		if (penetration < 0.f)
			return false;

		// Test a.z x b.z
		ra = a.radius.x * absR.m12 + a.radius.y * absR.m02;
		rb = b.radius.x * absR.m21 + b.radius.y * absR.m20;
		penetration = ra + rb - abs(t.y * r.m02 - t.x * r.m12);
		if (penetration < 0.f)
			return false;

		return true;
	}

	bool obbVsHull(const bounding_oriented_box& o, const bounding_hull& h)
	{
		obb_support_fn obbSupport{ o };
		hull_support_fn hullSupport{ h };

		gjk_simplex gjkSimplex;
		return gjkIntersectionTest(obbSupport, hullSupport, gjkSimplex);
	}

	bool obbVsPlane(const bounding_oriented_box& a, vec4 plane)
	{
		vec3 center = a.center;
		vec3 radius = a.radius;

		vec3 e0 = a.rotation * vec3(1.f, 0.f, 0.f);
		vec3 e1 = a.rotation * vec3(0.f, 1.f, 0.f);
		vec3 e2 = a.rotation * vec3(0.f, 0.f, 1.f);

		float r = radius.x * abs(dot(plane.xyz, e0))
			+ radius.y * abs(dot(plane.xyz, e1))
			+ radius.z * abs(dot(plane.xyz, e2));

		float s = abs(signedDistanceToPlane(center, plane));

		return s <= r;
	}

	bool obbVsTriangle(const bounding_oriented_box& obb, vec3 a, vec3 b, vec3 c)
	{
		a = conjugate(obb.rotation) * (a - obb.center);
		b = conjugate(obb.rotation) * (b - obb.center);
		c = conjugate(obb.rotation) * (c - obb.center);

		return aabbVsTriangle({ -obb.radius, obb.radius }, a, b, c);
	}

	bool hullVsHull(const bounding_hull& a, const bounding_hull& b)
	{
		hull_support_fn hullSupport1{ a };
		hull_support_fn hullSupport2{ b };

		gjk_simplex gjkSimplex;
		return gjkIntersectionTest(hullSupport1, hullSupport2, gjkSimplex);
	}

	float closestPoint_SegmentSegment(const line_segment& l1, const line_segment& l2, vec3& c1, vec3& c2)
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

	vec3 closestPoint_PointTriangle(const vec3& p, const vec3& a, const vec3& b, const vec3& c)
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

	static void addBoundingHullEdge(uint32 a, uint32 b, uint32 face, std::unordered_map<uint32, bounding_hull_edge*>& edgeMap, std::vector<bounding_hull_edge>& edges, uint32& edgeIndex)
	{
		uint32 from = min(a, b);
		uint32 to = max(a, b);
		uint32 edge = (from << 16) | to;

		auto it = edgeMap.find(edge);
		if (it == edgeMap.end())
		{
			auto& newEdge = edges[edgeIndex++];
			newEdge.from = from;
			newEdge.to = to;
			newEdge.faceA = face;
			edgeMap.insert({ edge, &newEdge });
		}
		else
		{
			ASSERT(it->second->from == from);
			ASSERT(it->second->to == to);
			ASSERT(it->second->faceA != UINT16_MAX);
			ASSERT(it->second->faceB == UINT16_MAX);
			it->second->faceB = face;
		}
	}

	template <typename triangle_t>
	static bounding_hull_geometry hullFromMesh(vec3* vertices, uint32 numVertices, triangle_t* triangles, uint32 numTriangles)
	{
		bounding_hull_geometry hull;

		uint32 numEdges = numVertices + numTriangles - 2; // Euler characteristic for convex polyhedra

		hull.vertices.resize(numVertices);
		hull.faces.resize(numTriangles);
		hull.edges.resize(numEdges, { UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX });
		hull.aabb = bounding_box::negativeInfinity();

		for (uint32 i = 0; i < numVertices; ++i)
		{
			hull.vertices[i] = vertices[i];
			hull.aabb.grow(vertices[i]);
		}

		std::unordered_map<uint32, bounding_hull_edge*> edgeMap;

		uint32 edgeIndex = 0;

		for (uint32 i = 0; i < numTriangles; ++i)
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

			addBoundingHullEdge(a, b, i, edgeMap, hull.edges, edgeIndex);
			addBoundingHullEdge(b, c, i, edgeMap, hull.edges, edgeIndex);
			addBoundingHullEdge(a, c, i, edgeMap, hull.edges, edgeIndex);
		}

		ASSERT(edgeIndex == numEdges);

		return hull;
	}

	bounding_hull_geometry bounding_hull_geometry::fromMesh(vec3* vertices, uint32 numVertices, indexed_triangle16* triangles, uint32 numTriangles)
	{
		return hullFromMesh(vertices, numVertices, triangles, numTriangles);
	}

	bounding_hull_geometry bounding_hull_geometry::fromMesh(vec3* vertices, uint32 numVertices, indexed_triangle32* triangles, uint32 numTriangles)
	{
		return hullFromMesh(vertices, numVertices, triangles, numTriangles);
	}

}