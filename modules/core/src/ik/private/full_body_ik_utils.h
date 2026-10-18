#pragma once

#include "core/math.h"

namespace era_engine::ik_utils
{
    inline quat swing_rotation(const vec3& from, const vec3& to, const vec3& fallback_axis)
    {
        const float lengths_product = length(from) * length(to);
        if (lengths_product > EPSILON && dot(from, to) < (-1.0f + 1e-5f) * lengths_product)
        {
            const float fallback_length = length(fallback_axis);
            const vec3 axis = fallback_length > EPSILON ? fallback_axis / fallback_length : any_perpendicular(normalize(from));
            return quat(axis, M_PI);
        }
        return rotate_from_to(from, to);
    }

    inline float soft_distance(float distance, float soft_start_distance, float reach_distance)
    {
        if (distance <= soft_start_distance)
        {
            return distance;
        }

        const float soft_length = reach_distance - soft_start_distance;
        if (soft_length <= EPSILON)
        {
            return std::min(distance, reach_distance);
        }

        return soft_start_distance + soft_length * (1.0f - std::exp(-(distance - soft_start_distance) / soft_length));
    }

    inline float smooth_clamp(float distance, float soft_start_distance, float max_distance)
    {
        if (distance <= soft_start_distance)
        {
            return distance;
        }

        const float soft_length = max_distance - soft_start_distance;
        if (soft_length <= EPSILON)
        {
            return std::min(distance, max_distance);
        }

        const float t = 1.0f - (distance - soft_start_distance) / (3.0f * soft_length);
        return t <= 0.0f ? max_distance : max_distance - soft_length * t * t * t;
    }

    inline vec3 transport(const vec3& v, const vec3& from, const vec3& to)
    {
        return v - (dot(v, to) / (1.0f + dot(from, to))) * (from + to);
    }

    inline float weighted_angle(float angle, float weight)
    {
        if (weight >= 1.0f)
        {
            return angle;
        }
        return angle - 2.0f * std::atan((1.0f - weight) * std::tan(0.5f * angle));
    }

    inline float twist_angle(quat rotation, const vec3& axis)
    {
        if (rotation.w < 0.0f)
        {
            rotation = -rotation;
        }
        return 2.0f * std::atan2(dot(rotation.v, axis), rotation.w);
    }

    inline vec3 rotation_vector(quat rotation)
    {
        if (rotation.w < 0.0f)
        {
            rotation = -rotation;
        }

        const float sin_half = length(rotation.v);
        if (sin_half < 1e-7f)
        {
            return rotation.v * 2.0f;
        }
        return rotation.v * (2.0f * std::atan2(sin_half, rotation.w) / sin_half);
    }
}
