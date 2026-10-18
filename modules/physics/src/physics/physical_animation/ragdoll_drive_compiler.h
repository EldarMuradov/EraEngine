#pragma once

#include "physics_api.h"

#include "physics/physical_animation/ragdoll_profile.h"

#include <core/math.h>

namespace era_engine::physics
{
    class PhysicalAnimationLimbComponent;

    struct ERA_PHYSICS_API LimbMassProperties final
    {
        bool valid = false;

        float mass = 0.0f;

        // Inertia along the design axis: the principal axis with the largest I_pivot / I_cm (a swing axis).
        float inertia_cm = 0.0f;
        float inertia_pivot = 0.0f;

        // Mean effective mass of the linear drive rows at the anchor.
        float linear_effective_mass = 0.0f;

        float get_inertia_ratio() const { return valid ? inertia_pivot / inertia_cm : 0.0f; }
        float get_linear_mass_ratio() const { return valid ? mass / linear_effective_mass : 0.0f; }
    };

    struct ERA_PHYSICS_API CompiledDriveValues final
    {
        static CompiledDriveValues lerp(const CompiledDriveValues& from, const CompiledDriveValues& to, float t);

        float stiffness = 0.0f;
        vec2 damping_range = vec2(0.0f);
        vec2 error_range = vec2(0.0f, 1.0f);
        float force_limit = 0.0f;
        bool accelerated = true;
    };

    struct ERA_PHYSICS_API CompiledLimbDrive final
    {
        static CompiledLimbDrive lerp(const CompiledLimbDrive& from, const CompiledLimbDrive& to, float t);

        CompiledDriveValues angular;
        CompiledDriveValues linear;

        float angular_frequency = 0.0f;          // About the joint, Hz.
        vec2 angular_damping_ratio = vec2(0.0f); // Near / far.
        float torque_stiffness = 0.0f;           // About the joint, N*m/rad.
        float max_torque = 0.0f;                 // N*m.

        float linear_frequency = 0.0f;          // Of the limb mass, Hz.
        vec2 linear_damping_ratio = vec2(0.0f); // Near / far.
        float max_force = 0.0f;                 // N.
    };

    // Acceleration drives (PxD6JointDriveFlag::eACCELERATION): PhysX divides the spring by the effective mass of the
    // drive row, computed from the two actors of the drive joint only. The kinematic target contributes nothing, so
    // the row sees the limb as a free body: angular rows use I_cm (inertia about the centre of mass), linear rows use
    // m_eff (mass at the anchor). Then alpha = k * theta for a free limb, w^2 = k.
    //
    // The limb is not free: it is held at its joint by the parent constraint, so it rotates about the joint with
    // I_pivot = I_cm + m * d^2 and the drive acceleration drops by I_cm / I_pivot. The compiler compensates with a
    // dimensionless ratio that depends on the shape and the joint position only (mass cancels out):
    //
    //     Angular: k = w^2 * (I_pivot / I_cm),   c = 2 * zeta * w * (I_pivot / I_cm)
    //     Linear:  k = w^2 * (m / m_eff),        c = 2 * zeta * w * (m / m_eff)
    //              m / m_eff = 1 + m * (r x n)^T I^-1 (r x n), the anchor is held by the chain, so the drive moves the limb mass.
    //     w = 2 * pi * f. E.g. a capsule driven at its end: I_pivot / I_cm = 4, k = 4 * w^2.
    //
    // Only force limits depend on mass (PhysX limits are forces/torques even for acceleration drives):
    //     max torque = I_pivot * w^2 * saturation_angle, max force = m * w^2 * saturation_distance.
    class ERA_PHYSICS_API RagdollDriveCompiler final
    {
    public:
        RagdollDriveCompiler() = delete;

        static LimbMassProperties compute_limb_mass_properties(const PhysicalAnimationLimbComponent* limb_component);

        static CompiledLimbDrive compile_limb_drive(const MotorDriveSettings& settings, const LimbMassProperties& mass_properties, float strength);
    };
}
