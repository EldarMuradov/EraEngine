#pragma once

#include "motion_matching_api.h"

#include "motion_matching/vec.h"

namespace era_engine
{

    struct ERA_MOTION_MATCHING_API Quat
    {
        Quat() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
        Quat(float _w, float _x, float _y, float _z) : w(_w), x(_x), y(_y), z(_z) {}

        float w, x, y, z;
    };

    static inline Quat operator*(Quat q, float s)
    {
        return Quat(q.w * s, q.x * s, q.y * s, q.z * s);
    }

    static inline Quat operator*(float s, Quat q)
    {
        return Quat(q.w * s, q.x * s, q.y * s, q.z * s);
    }

    static inline Quat operator+(Quat q, Quat p)
    {
        return Quat(q.w + p.w, q.x + p.x, q.y + p.y, q.z + p.z);
    }

    static inline Quat operator-(Quat q, Quat p)
    {
        return Quat(q.w - p.w, q.x - p.x, q.y - p.y, q.z - p.z);
    }

    static inline Quat operator/(Quat q, float s)
    {
        return Quat(q.w / s, q.x / s, q.y / s, q.z / s);
    }

    static inline Quat operator-(Quat q)
    {
        return Quat(-q.w, -q.x, -q.y, -q.z);
    }

    static inline float quat_length(Quat q)
    {
        return sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    }

    static inline Quat quat_normalize(Quat q, const float eps = 1e-8f)
    {
        return q / (quat_length(q) + eps);
    }

    static inline Quat quat_inv(Quat q)
    {
        return Quat(-q.w, q.x, q.y, q.z);
    }

    static inline Quat quat_mul(Quat q, Quat p)
    {
        return Quat(
            p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z,
            p.w * q.x + p.x * q.w - p.y * q.z + p.z * q.y,
            p.w * q.y + p.x * q.z + p.y * q.w - p.z * q.x,
            p.w * q.z - p.x * q.y + p.y * q.x + p.z * q.w);
    }

    static inline Quat quat_inv_mul(Quat q, Quat p)
    {
        return quat_mul(quat_inv(q), p);
    }

    static inline Quat quat_mul_inv(Quat q, Quat p)
    {
        return quat_mul(q, quat_inv(p));
    }

    static inline Vec3 quat_mul_vec3(Quat q, Vec3 v)
    {
        Vec3 t = 2.0f * cross(Vec3(q.x, q.y, q.z), v);
        return v + q.w * t + cross(Vec3(q.x, q.y, q.z), t);
    }

    static inline Vec3 quat_inv_mul_vec3(Quat q, Vec3 v)
    {
        return quat_mul_vec3(quat_inv(q), v);
    }

    static inline Quat quat_abs(Quat x)
    {
        return x.w < 0.0 ? -x : x;
    }

    static inline Quat quat_exp(Vec3 v, float eps = 1e-8f)
    {
        float halfangle = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

        if (halfangle < eps)
        {
            return quat_normalize(Quat(1.0f, v.x, v.y, v.z));
        }
        else
        {
            float c = cosf(halfangle);
            float s = sinf(halfangle) / halfangle;
            return Quat(c, s * v.x, s * v.y, s * v.z);
        }
    }

    static inline Vec3 quat_log(Quat q, float eps = 1e-8f)
    {
        float length = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z);

        if (length < eps)
        {
            return Vec3(q.x, q.y, q.z);
        }
        else
        {
            float halfangle = acosf(clampf(q.w, -1.0f, 1.0f));
            return halfangle * (Vec3(q.x, q.y, q.z) / length);
        }
    }

    static inline Quat quat_from_scaled_angle_axis(Vec3 v, float eps = 1e-8f)
    {
        return quat_exp(v / 2.0f, eps);
    }

    static inline Vec3 quat_to_scaled_angle_axis(Quat q, float eps = 1e-8f)
    {
        return 2.0f * quat_log(q, eps);
    }

    static inline Vec3 quat_differentiate_angular_velocity(
        Quat next, Quat curr, float dt, float eps = 1e-8f)
    {
        return quat_to_scaled_angle_axis(
            quat_abs(quat_mul(next, quat_inv(curr))), eps) / dt;
    }

    static inline Quat quat_integrate_angular_velocity(
        Vec3 vel, Quat curr, float dt, float eps = 1e-8f)
    {
        return quat_mul(quat_from_scaled_angle_axis(vel * dt, eps), curr);
    }

    static inline Quat quat_from_angle_axis(float angle, Vec3 axis)
    {
        float c = cosf(angle / 2.0f);
        float s = sinf(angle / 2.0f);
        return Quat(c, s * axis.x, s * axis.y, s * axis.z);
    }

    static inline void quat_to_angle_axis(Quat q, float& angle, Vec3& axis, float eps = 1e-8f)
    {
        float length = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z);

        if (length < eps)
        {
            angle = 0.0f;
            axis = Vec3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            angle = 2.0f * acosf(clampf(q.w, -1.0f, 1.0f));
            axis = Vec3(q.x, q.y, q.z) / length;
        }
    }

    static inline float quat_dot(Quat q, Quat p)
    {
        return q.w * p.w + q.x * p.x + q.y * p.y + q.z * p.z;
    }

    static inline Quat quat_nlerp(Quat q, Quat p, float alpha)
    {
        return quat_normalize(Quat(
            lerpf(q.w, p.w, alpha),
            lerpf(q.x, p.x, alpha),
            lerpf(q.y, p.y, alpha),
            lerpf(q.z, p.z, alpha)));
    }

    static inline Quat quat_nlerp_shortest(Quat q, Quat p, float alpha)
    {
        if (quat_dot(q, p) < 0.0f)
        {
            p = -p;
        }

        return quat_nlerp(q, p, alpha);
    }

    static inline Quat quat_slerp_shortest(Quat q, Quat p, float alpha, float eps = 1e-5f)
    {
        if (quat_dot(q, p) < 0.0f)
        {
            p = -p;
        }

        float dot = quat_dot(q, p);
        float theta = acosf(clampf(dot, -1.0f, 1.0f));

        if (theta < eps)
        {
            return quat_nlerp(q, p, alpha);
        }

        Quat r = quat_normalize(p - q * dot);

        return q * cosf(theta * alpha) + r * sinf(theta * alpha);
    }

    // Taken from https://zeux.io/2015/07/23/approximating-slerp/
    static inline Quat quat_slerp_shortest_approx(Quat q, Quat p, float alpha)
    {
        float ca = quat_dot(q, p);

        if (ca < 0.0f)
        {
            p = -p;
        }

        float d = fabsf(ca);
        float a = 1.0904f + d * (-3.2452f + d * (3.55645f - d * 1.43519f));
        float b = 0.848013f + d * (-1.06021f + d * 0.215638f);
        float k = a * (alpha - 0.5f) * (alpha - 0.5f) + b;
        float oalpha = alpha + alpha * (alpha - 0.5f) * (alpha - 1) * k;

        return quat_nlerp(q, p, oalpha);
    }

    static inline float quat_angle_between(Quat q, Quat p)
    {
        Quat diff = quat_abs(quat_mul_inv(q, p));
        return 2.0f * acosf(clampf(diff.w, -1.0f, 1.0f));
    }

    static inline Quat quat_between(Vec3 p, Vec3 q)
    {
        Vec3 c = cross(p, q);

        return quat_normalize(Quat(
            sqrtf(dot(p, p) * dot(q, q)) + dot(p, q),
            c.x,
            c.y,
            c.z));
    }

    static inline Quat quat_from_cols(Vec3 c0, Vec3 c1, Vec3 c2)
    {
        if (c2.z < 0.0f)
        {
            if (c0.x > c1.y)
            {
                return quat_normalize(Quat(
                    c1.z - c2.y,
                    1.0f + c0.x - c1.y - c2.z,
                    c0.y + c1.x,
                    c2.x + c0.z));
            }
            else
            {
                return quat_normalize(Quat(
                    c2.x - c0.z,
                    c0.y + c1.x,
                    1.0f - c0.x + c1.y - c2.z,
                    c1.z + c2.y));
            }
        }
        else
        {
            if (c0.x < -c1.y)
            {
                return quat_normalize(Quat(
                    c0.y - c1.x,
                    c2.x + c0.z,
                    c1.z + c2.y,
                    1.0f - c0.x - c1.y + c2.z));
            }
            else
            {
                return quat_normalize(Quat(
                    1.0f + c0.x + c1.y + c2.z,
                    c1.z - c2.y,
                    c2.x - c0.z,
                    c0.y - c1.x));
            }
        }
    }

    static inline Quat quat_from_xform_xy(Vec3 x, Vec3 y)
    {
        Vec3 c2 = normalize(cross(x, y));
        Vec3 c1 = normalize(cross(c2, x));
        Vec3 c0 = x;
        return quat_from_cols(c0, c1, c2);
    }

}