#pragma once

#include "motion_matching_api.h"

#include "motion_matching/common.h"

namespace era_engine
{

    struct ERA_MOTION_MATCHING_API Vec2
    {
        Vec2() : x(0.0f), y(0.0f) {}
        Vec2(float _x, float _y) : x(_x), y(_y) {}

        float x, y;
    };

    static inline Vec2 operator+(float s, Vec2 v)
    {
        return Vec2(v.x + s, v.y + s);
    }

    static inline Vec2 operator+(Vec2 v, float s)
    {
        return Vec2(v.x + s, v.y + s);
    }

    static inline Vec2 operator+(Vec2 v, Vec2 w)
    {
        return Vec2(v.x + w.x, v.y + w.y);
    }

    static inline Vec2 operator-(float s, Vec2 v)
    {
        return Vec2(s - v.x, s - v.y);
    }

    static inline Vec2 operator-(Vec2 v, float s)
    {
        return Vec2(v.x - s, v.y - s);
    }

    static inline Vec2 operator-(Vec2 v, Vec2 w)
    {
        return Vec2(v.x - w.x, v.y - w.y);
    }

    static inline Vec2 operator*(float s, Vec2 v)
    {
        return Vec2(v.x * s, v.y * s);
    }

    static inline Vec2 operator*(Vec2 v, float s)
    {
        return Vec2(v.x * s, v.y * s);
    }

    static inline Vec2 operator*(Vec2 v, Vec2 w)
    {
        return Vec2(v.x * w.x, v.y * w.y);
    }

    static inline Vec2 operator/(Vec2 v, float s)
    {
        return Vec2(v.x / s, v.y / s);
    }

    static inline Vec2 operator/(float s, Vec2 v)
    {
        return Vec2(s / v.x, s / v.y);
    }

    static inline Vec2 operator/(Vec2 v, Vec2 w)
    {
        return Vec2(v.x / w.x, v.y / w.y);
    }

    static inline Vec2 operator-(Vec2 v)
    {
        return Vec2(-v.x, -v.y);
    }

    static inline float dot(Vec2 v, Vec2 w)
    {
        return v.x * w.x + v.y * w.y;
    }

    static inline float length(Vec2 v)
    {
        return sqrtf(v.x * v.x + v.y * v.y);
    }

    static inline Vec2 normalize(Vec2 v, float eps = 1e-8f)
    {
        return v / (length(v) + eps);
    }

    static inline Vec2 lerp(Vec2 v, Vec2 w, float alpha)
    {
        return v * (1.0f - alpha) + w * alpha;
    }

    //--------------------------------------

    struct ERA_MOTION_MATCHING_API Vec3
    {
        Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
        Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

        float x, y, z;
    };

    static inline Vec3 operator+(float s, Vec3 v)
    {
        return Vec3(v.x + s, v.y + s, v.z + s);
    }

    static inline Vec3 operator+(Vec3 v, float s)
    {
        return Vec3(v.x + s, v.y + s, v.z + s);
    }

    static inline Vec3 operator+(Vec3 v, Vec3 w)
    {
        return Vec3(v.x + w.x, v.y + w.y, v.z + w.z);
    }

    static inline Vec3 operator-(float s, Vec3 v)
    {
        return Vec3(s - v.x, s - v.y, s - v.z);
    }

    static inline Vec3 operator-(Vec3 v, float s)
    {
        return Vec3(v.x - s, v.y - s, v.z - s);
    }

    static inline Vec3 operator-(Vec3 v, Vec3 w)
    {
        return Vec3(v.x - w.x, v.y - w.y, v.z - w.z);
    }

    static inline Vec3 operator*(float s, Vec3 v)
    {
        return Vec3(v.x * s, v.y * s, v.z * s);
    }

    static inline Vec3 operator*(Vec3 v, float s)
    {
        return Vec3(v.x * s, v.y * s, v.z * s);
    }

    static inline Vec3 operator*(Vec3 v, Vec3 w)
    {
        return Vec3(v.x * w.x, v.y * w.y, v.z * w.z);
    }

    static inline Vec3 operator/(Vec3 v, float s)
    {
        return Vec3(v.x / s, v.y / s, v.z / s);
    }

    static inline Vec3 operator/(float s, Vec3 v)
    {
        return Vec3(s / v.x, s / v.y, s / v.z);
    }

    static inline Vec3 operator/(Vec3 v, Vec3 w)
    {
        return Vec3(v.x / w.x, v.y / w.y, v.z / w.z);
    }

    static inline Vec3 operator-(Vec3 v)
    {
        return Vec3(-v.x, -v.y, -v.z);
    }

    static inline float dot(Vec3 v, Vec3 w)
    {
        return v.x * w.x + v.y * w.y + v.z * w.z;
    }

    static inline Vec3 cross(Vec3 v, Vec3 w)
    {
        return Vec3(
            v.y * w.z - v.z * w.y,
            v.z * w.x - v.x * w.z,
            v.x * w.y - v.y * w.x);
    }

    static inline float length(Vec3 v)
    {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    static inline Vec3 normalize(Vec3 v, float eps = 1e-8f)
    {
        return v / (length(v) + eps);
    }

    static inline Vec3 lerp(Vec3 v, Vec3 w, float alpha)
    {
        return v * (1.0f - alpha) + w * alpha;
    }

    static inline Vec3 clamp_Vec3(Vec3 v, Vec3 min, Vec3 max)
    {
        return Vec3(
            clampf(v.x, min.x, max.x),
            clampf(v.y, min.y, max.y),
            clampf(v.z, min.z, max.z));
    }

}