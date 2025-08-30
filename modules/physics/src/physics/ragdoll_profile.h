#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include <core/math.h>

#include <animation/animation.h>

#include <ecs/component.h>

namespace era_engine::physics
{
    enum class ConstraintBlendType : uint8
    {
        NONE = 0,
        BLEND_WITH_PREV_POSE,
        BLEND_WITH_ANIMATION_POSE,
        PURE_PHYSICS,
        PURE_ANIMATION
    };
    DEFINE_BITWISE_OPERATORS_FOR_ENUM(ConstraintBlendType);

    class ERA_PHYSICS_API ConstraintDetails final
    {
    public:
        ConstraintDetails() = default;
        ConstraintDetails(const ConstraintDetails& other) = default;
        ConstraintDetails(ConstraintDetails&& other) = default;

        ConstraintDetails& operator=(const ConstraintDetails& other) = default;

        float drive_damping = 20.0f;
        float drive_stiffness = 350.0f;

        float max_force = std::numeric_limits<float>::max();

        float drive_velocity_modifier = 1.0f;

        bool enable_slerp_drive = true;

        ConstraintBlendType blend_type = ConstraintBlendType::BLEND_WITH_PREV_POSE;
    };

    enum class RagdollProfileType : uint8
    {
        IDLE = 0,
        RUNNING,
        SPRING,
        CLIMBING,
        RAGDOLL
    };

    // Values for default idle profile.
    class ERA_PHYSICS_API RagdollProfile final
    {
    public:
        ConstraintDetails head_constraint;
        ConstraintDetails neck_constraint;

        ConstraintDetails body_upper_constraint;
        ConstraintDetails body_middle_constraint;

        ConstraintDetails arm_constraint;
        ConstraintDetails forearm_constraint;
        ConstraintDetails hand_constraint;

        ConstraintDetails leg_constraint;
        ConstraintDetails calf_constraint;
        ConstraintDetails foot_constraint;

        float acceleration_limit = 0.2f;
        float acceleration_gain = 0.02f;
        float legs_acceleration_gain = 0.02f;

        float partial_velocity_drive_limit = 1.0f;
        float partial_velocity_drive = 0.05f;

        float drive_angular_velocity_modifier = 0.95f;

        float legs_partial_angular_drive = 0.55f;
        float partial_angular_drive = 0.55f;
        float partial_angular_drive_limit = 25.0f;

        RagdollProfileType type = RagdollProfileType::IDLE;
    };
}