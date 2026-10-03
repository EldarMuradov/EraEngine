#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"
#include "physics/ragdolls/ragdoll_component.h"

#include <core/math.h>

#include <animation/animation.h>

#include <ecs/component.h>

namespace era_engine::physics
{
    enum class SkeletonUpdateType : uint8
    {
        NONE = 0,
        ROTATION = 1 << 0,
        TRANSLATION = 1 << 1,
        TRANSFORM = ROTATION | TRANSLATION
    };
    DEFINE_BITWISE_OPERATORS_FOR_ENUM(SkeletonUpdateType);

    enum class PhysicalLimbBlendType : uint8
    {
        NONE = 0,
        BLEND_WITH_PREV_POSE = 1 << 0,
        BLEND_WITH_ANIMATION_POSE = 1 << 1,
        PURE_PHYSICS = 1 << 2,
        PURE_ANIMATION = 1 << 3
    };
    DEFINE_BITWISE_OPERATORS_FOR_ENUM(PhysicalLimbBlendType);

    struct ERA_PHYSICS_API AngularDriveSettings final
    {
        float frequency = 6.0f; // Hz

        float damping_ratio = 0.85f; // 1 = critical
        float far_damping_ratio = 0.4f;

        vec2 error_range_deg = vec2(3.0f, 30.0f); // [damping_ratio, far_damping_ratio].

        float saturation_angle_deg = 60.0f;
    };

    struct ERA_PHYSICS_API LinearDriveSettings final
    {
        float frequency = 4.0f; // Hz

        float damping_ratio = 1.0f; // 1 = critical
        float far_damping_ratio = 0.5f;

        vec2 error_range = vec2(0.05f, 0.5f); // [damping_ratio, far_damping_ratio]

        float saturation_distance = 0.3f;
    };

    struct ERA_PHYSICS_API MotorDriveSettings final
    {
        AngularDriveSettings angular;
        LinearDriveSettings linear;
    };

    class ERA_PHYSICS_API DragForceDetails final
    {
    public:
        float partial_angular_drive_limit = 100.0f;
        float partial_angular_drive = 0.5f;

        float partial_velocity_drive_limit = 100.0f;
        float partial_velocity_drive = 0.5f;

        float acceleration_limit = 0.2f;
        float acceleration_gain = 0.02f;
    };

    // Multiplier of the drive stiffness. Damping is scaled by sqrt(strength).
    struct ERA_PHYSICS_API LimbStrengthDetails final
    {
        float default_strength_coeff = 1.0f;

        float soft_strength_coeff = 0.5f;
        float hard_strength_coeff = 1.5f;
    };

    class ERA_PHYSICS_API PhysicalLimbDetails final
    {
    public:
        std::optional<MotorDriveSettings> motor_drive;
        std::optional<DragForceDetails> drag_force;

        LimbStrengthDetails strength_details;

        PhysicalLimbBlendType blend_type = PhysicalLimbBlendType::BLEND_WITH_PREV_POSE;
    };

    enum class RagdollProfileType : uint8
    {
        IDLE = 0,
        RUNNING,
        SPRINT,
        CLIMBING,
        RAGDOLL,

        COUNT,

        DEFAULT = IDLE
    };

    enum class RagdollProfileStrengthType : uint8
    {
        DEFAULT = 0,
        SOFT,
        HARD
    };

    class ERA_PHYSICS_API RagdollProfile final
    {
    public:
        static constexpr uint32 LIMB_TYPES_COUNT = static_cast<uint32>(RagdollLimbType::FOOT) + 1u;

        const PhysicalLimbDetails& get_limb_details_by_type(RagdollLimbType limb_type) const;
        PhysicalLimbDetails& get_limb_details_by_type(RagdollLimbType limb_type);

        float get_strength(RagdollLimbType limb_type, RagdollProfileStrengthType strength_type) const;

        PhysicalLimbDetails head_limb_details;
        PhysicalLimbDetails neck_limb_details;

        PhysicalLimbDetails body_upper_limb_details;
        PhysicalLimbDetails body_middle_limb_details;
        PhysicalLimbDetails body_lower_limb_details;

        PhysicalLimbDetails clavicle_limb_details;
        PhysicalLimbDetails arm_limb_details;
        PhysicalLimbDetails forearm_limb_details;
        PhysicalLimbDetails hand_limb_details;

        PhysicalLimbDetails leg_limb_details;
        PhysicalLimbDetails calf_limb_details;
        PhysicalLimbDetails foot_limb_details;

        RagdollProfileType type = RagdollProfileType::IDLE;
    };
}
