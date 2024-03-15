#pragma once

#include "core/math.h"

struct indexed_triangle16
{
	uint16 a, b, c;
};

struct indexed_triangle32
{
	uint32 a, b, c;
};

struct indexed_line16
{
	uint16 a, b;
};

struct indexed_line32
{
	uint32 a, b;
};

struct index_point32
{
	uint32 a;
};

struct index_point16
{
	uint16 a;
};

struct line_segment
{
	vec3 a, b;
};

struct bounding_sphere
{
	vec3 center;
	float radius;

	float volume()
	{
		float sqRadius = radius * radius;
		float sqRadiusPI = M_PI * sqRadius;
		float sphereVolume = 4.f / 3.f * sqRadiusPI * radius;
		return sphereVolume;
	}
};

struct bounding_capsule
{
	vec3 positionA;
	vec3 positionB;
	float radius;

	float volume()
	{
		float sqRadius = radius * radius;
		float sqRadiusPI = M_PI * sqRadius;
		float sphereVolume = 4.f / 3.f * sqRadiusPI * radius;
		float height = length(positionA - positionB);
		float cylinderVolume = sqRadiusPI * height;
		return sphereVolume + cylinderVolume;
	}
};

struct bounding_cylinder
{
	vec3 positionA;
	vec3 positionB;
	float radius;

	float volume()
	{
		float sqRadiusPI = M_PI * radius * radius;
		float height = length(positionA - positionB);
		float cylinderVolume = sqRadiusPI * height;
		return cylinderVolume;
	}
};

struct bounding_torus
{
	vec3 position;
	vec3 upAxis;
	float majorRadius;
	float tubeRadius;
};

union bounding_box_corners
{
	struct
	{
		vec3 i;
		vec3 x;
		vec3 y;
		vec3 xy;
		vec3 z;
		vec3 xz;
		vec3 yz;
		vec3 xyz;
	};
	vec3 corners[8];

	bounding_box_corners() {}
};

struct bounding_oriented_box;

struct bounding_box
{
	vec3 minCorner;
	vec3 maxCorner;

	void grow(vec3 o);
	void pad(vec3 p);
	NODISCARD vec3 getCenter() const;
	NODISCARD vec3 getRadius() const;
	NODISCARD bool contains(vec3 p) const;

	NODISCARD bounding_box transformToAABB(quat rotation, vec3 translation) const;
	NODISCARD bounding_oriented_box transformToOBB(quat rotation, vec3 translation) const;
	NODISCARD bounding_box_corners getCorners() const;
	NODISCARD bounding_box_corners getCorners(quat rotation, vec3 translation) const;

	NODISCARD static bounding_box everything();
	NODISCARD static bounding_box negativeInfinity();
	NODISCARD static bounding_box fromMinMax(vec3 minCorner, vec3 maxCorner);
	NODISCARD static bounding_box fromCenterRadius(vec3 center, vec3 radius);

	NODISCARD float volume()
	{
		vec3 diameter = maxCorner - minCorner;
		return diameter.x * diameter.y * diameter.z;
	}
};

struct bounding_oriented_box
{
	quat rotation;
	vec3 center;
	vec3 radius;

	NODISCARD float volume()
	{
		vec3 diameter = radius * 2.f;
		return diameter.x * diameter.y * diameter.z;
	}

	NODISCARD bounding_box getAABB() const;
	NODISCARD bounding_box transformToAABB(quat rotation, vec3 translation) const;
	NODISCARD bounding_oriented_box transformToOBB(quat rotation, vec3 translation) const;
	NODISCARD bounding_box_corners getCorners() const;
};

struct bounding_rectangle
{
	vec2 minCorner;
	vec2 maxCorner;

	void grow(vec2 o);
	void pad(vec2 p);
	vec2 getCenter() const;
	vec2 getRadius() const;
	bool contains(vec2 p) const;

	NODISCARD static bounding_rectangle negativeInfinity();
	NODISCARD static bounding_rectangle fromMinMax(vec2 minCorner, vec2 maxCorner);
	NODISCARD static bounding_rectangle fromCenterRadius(vec2 center, vec2 radius);
};

inline NODISCARD vec4 createPlane(vec3 point, vec3 normal)
{
	float d = -dot(normal, point);
	return vec4(normal, d);
}

inline NODISCARD vec3 createLine(vec2 point, vec2 normal)
{
	float d = -dot(normal, point);
	return vec3(normal, d);
}

NODISCARD float signedDistanceToPlane(const vec3& p, const vec4& plane);

struct bounding_plane
{
	vec4 plane;

	bounding_plane() {}
	bounding_plane(vec3 point, vec3 normal)
	{
		plane = createPlane(point, normal);
	}

	NODISCARD float signedDistance(vec3 p) const
	{
		return signedDistanceToPlane(p, plane);
	}
};

struct bounding_hull_edge
{
	uint16 from, to;
	uint16 faceA, faceB;
};

struct bounding_hull_face
{
	uint16 a, b, c;
	vec3 normal;
};

struct bounding_hull_geometry
{
	std::vector<vec3> vertices;
	std::vector<bounding_hull_edge> edges;
	std::vector<bounding_hull_face> faces;

	bounding_box aabb;

	NODISCARD static bounding_hull_geometry fromMesh(vec3* vertices, uint32 numVertices, indexed_triangle16* triangles, uint32 numTriangles);
	NODISCARD static bounding_hull_geometry fromMesh(vec3* vertices, uint32 numVertices, indexed_triangle32* triangles, uint32 numTriangles);
};

// MUST be convex.
struct bounding_hull
{
	quat rotation;
	vec3 position;

	union
	{
		uint32 geometryIndex;						// For permanent colliders.
		const bounding_hull_geometry* geometryPtr;	// For world space colliders, which only exist for one frame.
	};
};

struct ray
{
	vec3 origin;
	vec3 direction;

	NODISCARD bool intersectPlane(vec3 normal, float d, float& outT) const;
	NODISCARD bool intersectPlane(vec3 normal, vec3 point, float& outT) const;
	NODISCARD bool intersectAABB(const bounding_box& a, float& outT) const;
	NODISCARD bool intersectOBB(const bounding_oriented_box& a, float& outT) const;
	NODISCARD bool intersectTriangle(vec3 a, vec3 b, vec3 c, float& outT, bool& outFrontFacing) const;
	NODISCARD bool intersectSphere(vec3 center, float radius, float& outT) const;
	NODISCARD bool intersectSphere(const bounding_sphere& sphere, float& outT) const { return intersectSphere(sphere.center, sphere.radius, outT); }
	NODISCARD bool intersectCylinder(const bounding_cylinder& cylinder, float& outT) const;
	NODISCARD bool intersectCapsule(const bounding_capsule& capsule, float& outT) const;
	NODISCARD bool intersectDisk(vec3 pos, vec3 normal, float radius, float& outT) const;
	NODISCARD bool intersectRectangle(vec3 pos, vec3 tangent, vec3 bitangent, vec2 radius, float& outT) const;
	NODISCARD bool intersectTorus(const bounding_torus& torus, float& outT) const;
	NODISCARD bool intersectHull(const bounding_hull& hull, const bounding_hull_geometry& geometry, float& outT) const;
};

NODISCARD bool sphereVsSphere(const bounding_sphere& a, const bounding_sphere& b);
NODISCARD bool sphereVsPlane(const bounding_sphere& s, const vec4& p);
NODISCARD bool sphereVsCapsule(const bounding_sphere& s, const bounding_capsule& c);
NODISCARD bool sphereVsCylinder(const bounding_sphere& s, const bounding_cylinder& c);
NODISCARD bool sphereVsAABB(const bounding_sphere& s, const bounding_box& a);
NODISCARD bool sphereVsOBB(const bounding_sphere& s, const bounding_oriented_box& o);
NODISCARD bool sphereVsHull(const bounding_sphere& s, const bounding_hull& h);
NODISCARD bool sphereVsTriangle(const bounding_sphere& s, vec3 a, vec3 b, vec3 c);

NODISCARD bool capsuleVsCapsule(const bounding_capsule& a, const bounding_capsule& b);
NODISCARD bool capsuleVsCylinder(const bounding_capsule& a, const bounding_cylinder& b);
NODISCARD bool capsuleVsAABB(const bounding_capsule& c, const bounding_box& b);
NODISCARD bool capsuleVsOBB(const bounding_capsule& c, const bounding_oriented_box& o);
NODISCARD bool capsuleVsHull(const bounding_capsule& c, const bounding_hull& h);
NODISCARD bool capsuleVsTriangle(const bounding_capsule& capsule, vec3 a, vec3 b, vec3 c);

NODISCARD bool cylinderVsCylinder(const bounding_cylinder& a, const bounding_cylinder& b);
NODISCARD bool cylinderVsAABB(const bounding_cylinder& c, const bounding_box& b);
NODISCARD bool cylinderVsOBB(const bounding_cylinder& c, const bounding_oriented_box& o);
NODISCARD bool cylinderVsHull(const bounding_cylinder& c, const bounding_hull& h);

NODISCARD bool aabbVsAABB(const bounding_box& a, const bounding_box& b);
NODISCARD bool aabbVsOBB(const bounding_box& a, const bounding_oriented_box& o);
NODISCARD bool aabbVsHull(const bounding_box& a, const bounding_hull& h);
NODISCARD bool aabbVsPlane(const bounding_box& a, vec4 plane);
NODISCARD bool aabbVsTriangle(const bounding_box& aabb, vec3 a, vec3 b, vec3 c);

NODISCARD bool obbVsOBB(const bounding_oriented_box& a, const bounding_oriented_box& b);
NODISCARD bool obbVsHull(const bounding_oriented_box& o, const bounding_hull& h);
NODISCARD bool obbVsPlane(const bounding_oriented_box& a, vec4 plane);
NODISCARD bool obbVsTriangle(const bounding_oriented_box& obb, vec3 a, vec3 b, vec3 c);

NODISCARD bool hullVsHull(const bounding_hull& a, const bounding_hull& b);

NODISCARD vec3 closestPoint_PointSegment(const vec3& q, const line_segment& l);
NODISCARD vec3 closestPoint_PointAABB(const vec3& q, const bounding_box& aabb);
NODISCARD float closestPoint_SegmentSegment(const line_segment& l1, const line_segment& l2, vec3& c1, vec3& c2);
NODISCARD vec3 closestPoint_PointTriangle(const vec3& q, const vec3& a, const vec3& b, const vec3& c);

// Inline functions:

inline NODISCARD float signedDistanceToPlane(const vec3& p, const vec4& plane)
{
	return dot(vec4(p, 1.f), plane);
}

inline NODISCARD bool sphereVsSphere(const bounding_sphere& a, const bounding_sphere& b)
{
	vec3 d = a.center - b.center;
	float dist2 = dot(d, d);
	float radiusSum = a.radius + b.radius;
	return dist2 <= radiusSum * radiusSum;
}

inline NODISCARD bool sphereVsPlane(const bounding_sphere& s, const vec4& p)
{
	return abs(signedDistanceToPlane(s.center, p)) <= s.radius;
}

inline NODISCARD bool sphereVsCapsule(const bounding_sphere& s, const bounding_capsule& c)
{
	vec3 closestPoint = closestPoint_PointSegment(s.center, line_segment{ c.positionA, c.positionB });
	return sphereVsSphere(s, bounding_sphere{ closestPoint, c.radius });
}

inline NODISCARD bool sphereVsAABB(const bounding_sphere& s, const bounding_box& a)
{
	vec3 p = closestPoint_PointAABB(s.center, a);
	vec3 n = p - s.center;
	float sqDistance = squaredLength(n);
	return sqDistance <= s.radius * s.radius;
}

inline NODISCARD bool sphereVsOBB(const bounding_sphere& s, const bounding_oriented_box& o)
{
	bounding_box aabb = bounding_box::fromCenterRadius(o.center, o.radius);
	bounding_sphere s_ = {
		conjugate(o.rotation) * (s.center - o.center) + o.center,
		s.radius };

	return sphereVsAABB(s_, aabb);
}

inline bool capsuleVsCapsule(const bounding_capsule& a, const bounding_capsule& b)
{
	vec3 closestPoint1, closestPoint2;
	closestPoint_SegmentSegment(line_segment{ a.positionA, a.positionB }, line_segment{ b.positionA, b.positionB }, closestPoint1, closestPoint2);
	return sphereVsSphere(bounding_sphere{ closestPoint1, a.radius }, bounding_sphere{ closestPoint2, b.radius });
}

inline bool capsuleVsCylinder(const bounding_capsule& a, const bounding_cylinder& b)
{
	vec3 closestPoint1, closestPoint2;
	closestPoint_SegmentSegment(line_segment{ a.positionA, a.positionB }, line_segment{ b.positionA, b.positionB }, closestPoint1, closestPoint2);
	return sphereVsCylinder(bounding_sphere{ closestPoint1, a.radius }, b);
}

inline bool aabbVsAABB(const bounding_box& a, const bounding_box& b)
{
	if (a.maxCorner.x < b.minCorner.x || a.minCorner.x > b.maxCorner.x) return false;
	if (a.maxCorner.y < b.minCorner.y || a.minCorner.y > b.maxCorner.y) return false;
	if (a.maxCorner.z < b.minCorner.z || a.minCorner.z > b.maxCorner.z) return false;
	return true;
}

inline NODISCARD bool aabbVsOBB(const bounding_box& a, const bounding_oriented_box& o)
{
	return obbVsOBB(bounding_oriented_box{ quat::identity, a.getCenter(), a.getRadius() }, o);
}

inline vec3 closestPoint_PointSegment(const vec3& q, const line_segment& l)
{
	vec3 ab = l.b - l.a;
	float t = dot(q - l.a, ab) / squaredLength(ab);
	t = clamp(t, 0.f, 1.f);
	return l.a + t * ab;
}

inline vec3 closestPoint_PointAABB(const vec3& q, const bounding_box& aabb)
{
	vec3 result;
	for (uint32 i = 0; i < 3; ++i)
	{
		float v = q.data[i];
		if (v < aabb.minCorner.data[i]) v = aabb.minCorner.data[i];
		if (v > aabb.maxCorner.data[i]) v = aabb.maxCorner.data[i];
		result.data[i] = v;
	}
	return result;
}