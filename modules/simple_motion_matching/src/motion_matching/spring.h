#pragma once

#include "motion_matching/common.h"
#include "motion_matching/vec.h"
#include "motion_matching/quat.h"

namespace era_engine
{

    static inline float damper_exact(float x, float g, float halflife, float dt, float eps = 1e-5f)
    {
        return lerpf(x, g, 1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
    }

    static inline Vec3 damper_exact(Vec3 x, Vec3 g, float halflife, float dt, float eps = 1e-5f)
    {
        return lerp(x, g, 1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
    }

    static inline Quat damper_exact(Quat x, Quat g, float halflife, float dt, float eps = 1e-5f)
    {
        return quat_slerp_shortest_approx(x, g, 1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
    }

    static inline float damp_adjustment_exact(float g, float halflife, float dt, float eps = 1e-5f)
    {
        return g * (1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
    }

    static inline Vec3 damp_adjustment_exact(Vec3 g, float halflife, float dt, float eps = 1e-5f)
    {
        return g * (1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
    }

    static inline Quat damp_adjustment_exact(Quat g, float halflife, float dt, float eps = 1e-5f)
    {
        return quat_slerp_shortest_approx(Quat(), g, 1.0f - fast_negexpf((LN2f * dt) / (halflife + eps)));
    }

    //--------------------------------------

    static inline float halflife_to_damping(float halflife, float eps = 1e-5f)
    {
        return (4.0f * LN2f) / (halflife + eps);
    }

    static inline float damping_to_halflife(float damping, float eps = 1e-5f)
    {
        return (4.0f * LN2f) / (damping + eps);
    }

    static inline float frequency_to_stiffness(float frequency)
    {
        return squaref(2.0f * PIf * frequency);
    }

    static inline float stiffness_to_frequency(float stiffness)
    {
        return sqrtf(stiffness) / (2.0f * PIf);
    }

    //--------------------------------------

    static inline void simple_spring_damper_exact(
        float& x,
        float& v,
        const float x_goal,
        const float halflife,
        const float dt)
    {
        float y = halflife_to_damping(halflife) / 2.0f;
        float j0 = x - x_goal;
        float j1 = v + j0 * y;
        float eydt = fast_negexpf(y * dt);

        x = eydt * (j0 + j1 * dt) + x_goal;
        v = eydt * (v - j1 * y * dt);
    }

    static inline void simple_spring_damper_exact(
        Vec3& x,
        Vec3& v,
        const Vec3 x_goal,
        const float halflife,
        const float dt)
    {
        float y = halflife_to_damping(halflife) / 2.0f;
        Vec3 j0 = x - x_goal;
        Vec3 j1 = v + j0 * y;
        float eydt = fast_negexpf(y * dt);

        x = eydt * (j0 + j1 * dt) + x_goal;
        v = eydt * (v - j1 * y * dt);
    }

    static inline void simple_spring_damper_exact(
        Quat& x,
        Vec3& v,
        const Quat x_goal,
        const float halflife,
        const float dt)
    {
        float y = halflife_to_damping(halflife) / 2.0f;

        Vec3 j0 = quat_to_scaled_angle_axis(quat_abs(quat_mul(x, quat_inv(x_goal))));
        Vec3 j1 = v + j0 * y;

        float eydt = fast_negexpf(y * dt);

        x = quat_mul(quat_from_scaled_angle_axis(eydt * (j0 + j1 * dt)), x_goal);
        v = eydt * (v - j1 * y * dt);
    }

    //--------------------------------------

    static inline void decay_spring_damper_exact(
        float& x,
        float& v,
        const float halflife,
        const float dt)
    {
        float y = halflife_to_damping(halflife) / 2.0f;
        float j1 = v + x * y;
        float eydt = fast_negexpf(y * dt);

        x = eydt * (x + j1 * dt);
        v = eydt * (v - j1 * y * dt);
    }

    static inline void decay_spring_damper_exact(
        Vec3& x,
        Vec3& v,
        const float halflife,
        const float dt)
    {
        float y = halflife_to_damping(halflife) / 2.0f;
        Vec3 j1 = v + x * y;
        float eydt = fast_negexpf(y * dt);

        x = eydt * (x + j1 * dt);
        v = eydt * (v - j1 * y * dt);
    }

    static inline void decay_spring_damper_exact(
        Quat& x,
        Vec3& v,
        const float halflife,
        const float dt)
    {
        float y = halflife_to_damping(halflife) / 2.0f;

        Vec3 j0 = quat_to_scaled_angle_axis(x);
        Vec3 j1 = v + j0 * y;

        float eydt = fast_negexpf(y * dt);

        x = quat_from_scaled_angle_axis(eydt * (j0 + j1 * dt));
        v = eydt * (v - j1 * y * dt);
    }

    //--------------------------------------

    static inline void inertialize_transition(
        Vec3& off_x,
        Vec3& off_v,
        const Vec3 src_x,
        const Vec3 src_v,
        const Vec3 dst_x,
        const Vec3 dst_v)
    {
        off_x = (src_x + off_x) - dst_x;
        off_v = (src_v + off_v) - dst_v;
    }

    static inline void inertialize_update(
        Vec3& out_x,
        Vec3& out_v,
        Vec3& off_x,
        Vec3& off_v,
        const Vec3 in_x,
        const Vec3 in_v,
        const float halflife,
        const float dt)
    {
        decay_spring_damper_exact(off_x, off_v, halflife, dt);
        out_x = in_x + off_x;
        out_v = in_v + off_v;
    }

    static inline void inertialize_transition(
        Quat& off_x,
        Vec3& off_v,
        const Quat src_x,
        const Vec3 src_v,
        const Quat dst_x,
        const Vec3 dst_v)
    {
        off_x = quat_abs(quat_mul(quat_mul(off_x, src_x), quat_inv(dst_x)));
        off_v = (off_v + src_v) - dst_v;
    }

    static inline void inertialize_update(
        Quat& out_x,
        Vec3& out_v,
        Quat& off_x,
        Vec3& off_v,
        const Quat in_x,
        const Vec3 in_v,
        const float halflife,
        const float dt)
    {
        decay_spring_damper_exact(off_x, off_v, halflife, dt);
        out_x = quat_mul(off_x, in_x);
        out_v = off_v + quat_mul_vec3(off_x, in_v);
    }

}