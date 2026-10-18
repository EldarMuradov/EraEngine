// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"
#include "core/serialization/binary_serializer.h"

namespace era_engine
{
    struct ERA_CORE_API IndexedTriangle16
    {
        uint16 a, b, c;

        ERA_BINARY_SERIALIZE(a, b, c)
    };

    struct ERA_CORE_API IndexedTriangle32
    {
        uint32 a, b, c;

        ERA_BINARY_SERIALIZE(a, b, c)
    };

    struct ERA_CORE_API IndexedLine16
    {
        uint16 a, b;

        ERA_BINARY_SERIALIZE(a, b)
    };

    struct ERA_CORE_API IndexedLine32
    {
        uint32 a, b;

        ERA_BINARY_SERIALIZE(a, b)
    };

    struct ERA_CORE_API IndexPoint32
    {
        uint32 a;

        ERA_BINARY_SERIALIZE(a)
    };

    struct ERA_CORE_API IndexPoint16
    {
        uint16 a;

        ERA_BINARY_SERIALIZE(a)
    };

    struct ERA_CORE_API LineSegment
    {
        vec3 a, b;

        ERA_BINARY_SERIALIZE(a, b)
    };

    struct ERA_CORE_API BoundingSphere
    {
        float volume() const
        {
            float sq_radius = radius * radius;
            float sq_radius_pi = M_PI * sq_radius;
            float sphere_volume = 4.f / 3.f * sq_radius_pi * radius;
            return sphere_volume;
        }

        vec3 center;
        float radius;

        ERA_BINARY_SERIALIZE(center, radius)
    };

    struct ERA_CORE_API BoundingCapsule
    {
        float volume() const
        {
            float sq_radius = radius * radius;
            float sq_radius_pi = M_PI * sq_radius;
            float sphere_volume = 4.f / 3.f * sq_radius_pi * radius;
            float height = length(position_a - position_b);
            float cylinder_volume = sq_radius_pi * height;
            return sphere_volume + cylinder_volume;
        }

        vec3 position_a;
        vec3 position_b;
        float radius;

        ERA_BINARY_SERIALIZE(position_a, position_b, radius)
    };

    struct ERA_CORE_API BoundingCylinder
    {
        float volume() const
        {
            float sq_radius_pi = M_PI * radius * radius;
            float height = length(position_a - position_b);
            float cylinder_volume = sq_radius_pi * height;
            return cylinder_volume;
        }

        vec3 position_a;
        vec3 position_b;
        float radius;

        ERA_BINARY_SERIALIZE(position_a, position_b, radius)
    };

    struct ERA_CORE_API BoundingTorus
    {
        vec3 position;
        vec3 up_axis;
        float major_radius;
        float tube_radius;

        ERA_BINARY_SERIALIZE(position, up_axis, major_radius, tube_radius)
    };

    union BoundingBoxCorners
    {
        BoundingBoxCorners() {}

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
    };

    struct BoundingOrientedBox;

    struct ERA_CORE_API BoundingBox
    {
        void grow(vec3 o);
        void pad(vec3 p);
        vec3 get_center() const;
        vec3 get_radius() const;
        bool contains(vec3 p) const;

        BoundingBox transform_to_aabb(quat rotation, vec3 translation) const;
        BoundingOrientedBox transform_to_obb(quat rotation, vec3 translation) const;
        BoundingBoxCorners get_corners() const;
        BoundingBoxCorners get_corners(quat rotation, vec3 translation) const;

        static BoundingBox everything();
        static BoundingBox negative_infinity();
        static BoundingBox from_min_max(vec3 min_corner, vec3 max_corner);
        static BoundingBox from_center_radius(vec3 center, vec3 radius);

        float volume() const
        {
            vec3 diameter = max_corner - min_corner;
            return diameter.x * diameter.y * diameter.z;
        }

        vec3 min_corner;
        vec3 max_corner;

        ERA_BINARY_SERIALIZE(min_corner, max_corner)
    };

    struct ERA_CORE_API BoundingOrientedBox
    {
        float volume() const
        {
            vec3 diameter = radius * 2.f;
            return diameter.x * diameter.y * diameter.z;
        }

        BoundingBox get_aabb() const;
        BoundingBox transform_to_aabb(quat rotation, vec3 translation) const;
        BoundingOrientedBox transform_to_obb(quat rotation, vec3 translation) const;
        BoundingBoxCorners get_corners() const;

        quat rotation;
        vec3 center;
        vec3 radius;

        ERA_BINARY_SERIALIZE(rotation, center, radius)
    };

    struct ERA_CORE_API BoundingRectangle
    {
        void grow(vec2 o);
        void pad(vec2 p);
        vec2 get_center() const;
        vec2 get_radius() const;
        bool contains(vec2 p) const;

        static BoundingRectangle negative_infinity();
        static BoundingRectangle from_min_max(vec2 min_corner, vec2 max_corner);
        static BoundingRectangle from_center_radius(vec2 center, vec2 radius);

        vec2 min_corner;
        vec2 max_corner;

        ERA_BINARY_SERIALIZE(min_corner, max_corner)
    };

    inline vec4 create_plane(vec3 point, vec3 normal)
    {
        float d = -dot(normal, point);
        return vec4(normal, d);
    }

    inline vec3 create_line(vec2 point, vec2 normal)
    {
        float d = -dot(normal, point);
        return vec3(normal, d);
    }

    float signed_distance_to_plane(const vec3& p, const vec4& plane);

    struct ERA_CORE_API BoundingPlane
    {
        BoundingPlane() = default;
        BoundingPlane(vec3 point, vec3 normal)
        {
            plane = create_plane(point, normal);
        }

        float signed_distance(vec3 p) const
        {
            return signed_distance_to_plane(p, plane);
        }

        vec4 plane;
    };

    struct ERA_CORE_API BoundingHullEdge
    {
        uint16 from, to;
        uint16 face_a, face_b;
    };

    struct ERA_CORE_API BoundingHullFace
    {
        uint16 a, b, c;
        vec3 normal;
    };

    struct ERA_CORE_API BoundingHullGeometry
    {
        static BoundingHullGeometry from_mesh(vec3* vertices, uint32 num_vertices, IndexedTriangle16* triangles, uint32 num_triangles);
        static BoundingHullGeometry from_mesh(vec3* vertices, uint32 num_vertices, IndexedTriangle32* triangles, uint32 num_triangles);

        std::vector<vec3> vertices;
        std::vector<BoundingHullEdge> edges;
        std::vector<BoundingHullFace> faces;

        BoundingBox aabb;
    };

    // MUST be convex.
    struct ERA_CORE_API BoundingHull
    {
        quat rotation;
        vec3 position;

        union
        {
            uint32 geometry_index;                    // For permanent colliders.
            const BoundingHullGeometry* geometry_ptr; // For world space colliders, which only exist for one frame.
        };
    };

    struct ERA_CORE_API ray
    {
        bool intersect_plane(vec3 normal, float d, float& out_t) const;
        bool intersect_plane(vec3 normal, vec3 point, float& out_t) const;
        bool intersect_aabb(const BoundingBox& a, float& out_t) const;
        bool intersect_obb(const BoundingOrientedBox& a, float& out_t) const;
        bool intersect_triangle(vec3 a, vec3 b, vec3 c, float& out_t, bool& out_front_facing) const;
        bool intersect_sphere(vec3 center, float radius, float& out_t) const;
        bool intersect_sphere(const BoundingSphere& sphere, float& out_t) const { return intersect_sphere(sphere.center, sphere.radius, out_t); }
        bool intersect_cylinder(const BoundingCylinder& cylinder, float& out_t) const;
        bool intersect_capsule(const BoundingCapsule& capsule, float& out_t) const;
        bool intersect_disk(vec3 pos, vec3 normal, float radius, float& out_t) const;
        bool intersect_rectangle(vec3 pos, vec3 tangent, vec3 bitangent, vec2 radius, float& out_t) const;
        bool intersect_torus(const BoundingTorus& torus, float& out_t) const;
        bool intersect_hull(const BoundingHull& hull, const BoundingHullGeometry& geometry, float& out_t) const;

        vec3 origin;
        vec3 direction;
    };

    bool sphere_vs_sphere(const BoundingSphere& a, const BoundingSphere& b);
    bool sphere_vs_plane(const BoundingSphere& s, const vec4& p);
    bool sphere_vs_capsule(const BoundingSphere& s, const BoundingCapsule& c);
    bool sphere_vs_cylinder(const BoundingSphere& s, const BoundingCylinder& c);
    bool sphere_vs_aabb(const BoundingSphere& s, const BoundingBox& a);
    bool sphere_vs_obb(const BoundingSphere& s, const BoundingOrientedBox& o);
    bool sphere_vs_hull(const BoundingSphere& s, const BoundingHull& h);
    bool sphere_vs_triangle(const BoundingSphere& s, vec3 a, vec3 b, vec3 c);

    bool capsule_vs_capsule(const BoundingCapsule& a, const BoundingCapsule& b);
    bool capsule_vs_cylinder(const BoundingCapsule& a, const BoundingCylinder& b);
    bool capsule_vs_aabb(const BoundingCapsule& c, const BoundingBox& b);
    bool capsule_vs_obb(const BoundingCapsule& c, const BoundingOrientedBox& o);
    bool capsule_vs_hull(const BoundingCapsule& c, const BoundingHull& h);
    bool capsule_vs_triangle(const BoundingCapsule& capsule, vec3 a, vec3 b, vec3 c);

    bool cylinder_vs_cylinder(const BoundingCylinder& a, const BoundingCylinder& b);
    bool cylinder_vs_aabb(const BoundingCylinder& c, const BoundingBox& b);
    bool cylinder_vs_obb(const BoundingCylinder& c, const BoundingOrientedBox& o);
    bool cylinder_vs_hull(const BoundingCylinder& c, const BoundingHull& h);

    bool aabb_vs_aabb(const BoundingBox& a, const BoundingBox& b);
    bool aabb_vs_obb(const BoundingBox& a, const BoundingOrientedBox& o);
    bool aabb_vs_hull(const BoundingBox& a, const BoundingHull& h);
    bool aabb_vs_plane(const BoundingBox& a, vec4 plane);
    bool aabb_vs_triangle(const BoundingBox& aabb, vec3 a, vec3 b, vec3 c);

    bool obb_vs_obb(const BoundingOrientedBox& a, const BoundingOrientedBox& b);
    bool obb_vs_hull(const BoundingOrientedBox& o, const BoundingHull& h);
    bool obb_vs_plane(const BoundingOrientedBox& a, vec4 plane);
    bool obb_vs_triangle(const BoundingOrientedBox& obb, vec3 a, vec3 b, vec3 c);

    bool hull_vs_hull(const BoundingHull& a, const BoundingHull& b);

    vec3 closest_point_point_segment(const vec3& q, const LineSegment& l);
    vec3 closest_point_point_aabb(const vec3& q, const BoundingBox& aabb);
    float closest_point_segment_segment(const LineSegment& l1, const LineSegment& l2, vec3& c1, vec3& c2);
    vec3 closest_point_point_triangle(const vec3& q, const vec3& a, const vec3& b, const vec3& c);

    // Inline functions:

    inline float signed_distance_to_plane(const vec3& p, const vec4& plane)
    {
        return dot(vec4(p, 1.f), plane);
    }

    inline bool sphere_vs_sphere(const BoundingSphere& a, const BoundingSphere& b)
    {
        vec3 d = a.center - b.center;
        float dist2 = dot(d, d);
        float radius_sum = a.radius + b.radius;
        return dist2 <= radius_sum * radius_sum;
    }

    inline bool sphere_vs_plane(const BoundingSphere& s, const vec4& p)
    {
        return abs(signed_distance_to_plane(s.center, p)) <= s.radius;
    }

    inline bool sphere_vs_capsule(const BoundingSphere& s, const BoundingCapsule& c)
    {
        vec3 closest_point = closest_point_point_segment(s.center, LineSegment{c.position_a, c.position_b});
        return sphere_vs_sphere(s, BoundingSphere{closest_point, c.radius});
    }

    inline bool sphere_vs_aabb(const BoundingSphere& s, const BoundingBox& a)
    {
        vec3 p = closest_point_point_aabb(s.center, a);
        vec3 n = p - s.center;
        float sq_distance = squared_length(n);
        return sq_distance <= s.radius * s.radius;
    }

    inline bool sphere_vs_obb(const BoundingSphere& s, const BoundingOrientedBox& o)
    {
        BoundingBox aabb = BoundingBox::from_center_radius(o.center, o.radius);
        BoundingSphere s_ = {
            conjugate(o.rotation) * (s.center - o.center) + o.center,
            s.radius};

        return sphere_vs_aabb(s_, aabb);
    }

    inline bool capsule_vs_capsule(const BoundingCapsule& a, const BoundingCapsule& b)
    {
        vec3 closest_point1, closest_point2;
        closest_point_segment_segment(LineSegment{a.position_a, a.position_b}, LineSegment{b.position_a, b.position_b}, closest_point1, closest_point2);
        return sphere_vs_sphere(BoundingSphere{closest_point1, a.radius}, BoundingSphere{closest_point2, b.radius});
    }

    inline bool capsule_vs_cylinder(const BoundingCapsule& a, const BoundingCylinder& b)
    {
        vec3 closest_point1, closest_point2;
        closest_point_segment_segment(LineSegment{a.position_a, a.position_b}, LineSegment{b.position_a, b.position_b}, closest_point1, closest_point2);
        return sphere_vs_cylinder(BoundingSphere{closest_point1, a.radius}, b);
    }

    inline bool aabb_vs_aabb(const BoundingBox& a, const BoundingBox& b)
    {
        if (a.max_corner.x < b.min_corner.x || a.min_corner.x > b.max_corner.x)
            return false;
        if (a.max_corner.y < b.min_corner.y || a.min_corner.y > b.max_corner.y)
            return false;
        if (a.max_corner.z < b.min_corner.z || a.min_corner.z > b.max_corner.z)
            return false;
        return true;
    }

    inline bool aabb_vs_obb(const BoundingBox& a, const BoundingOrientedBox& o)
    {
        return obb_vs_obb(BoundingOrientedBox{quat::identity, a.get_center(), a.get_radius()}, o);
    }

    inline vec3 closest_point_point_segment(const vec3& q, const LineSegment& l)
    {
        vec3 ab = l.b - l.a;
        float t = dot(q - l.a, ab) / squared_length(ab);
        t = clamp(t, 0.f, 1.f);
        return l.a + t * ab;
    }

    inline vec3 closest_point_point_aabb(const vec3& q, const BoundingBox& aabb)
    {
        vec3 result;
        for (uint32 i = 0; i < 3; ++i)
        {
            float v = q.data[i];
            if (v < aabb.min_corner.data[i])
                v = aabb.min_corner.data[i];
            if (v > aabb.max_corner.data[i])
                v = aabb.max_corner.data[i];
            result.data[i] = v;
        }
        return result;
    }
}