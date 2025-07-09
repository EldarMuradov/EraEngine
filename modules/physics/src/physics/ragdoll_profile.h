#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include <core/math.h>

#include <animation/animation.h>

#include <ecs/component.h>
#include <ecs/observable_member.h>

namespace era_engine::physics
{
    enum class ConstraintBlendType : uint8
    {
        BLEND_WITH_PREV_POSE = 0,
        BLEND_WITH_ADJUSTED_POSE,
        PURE_PHYSICS
    };

    class ERA_PHYSICS_API ConstraintDetails final
    {
    public:
        ConstraintDetails() = default;
        ConstraintDetails(const ConstraintDetails& other) = default;
        ConstraintDetails(ConstraintDetails&& other) = default;

        ConstraintDetails& operator=(const ConstraintDetails& other) = default;

        float drive_damping = 10.0f;
        float drive_stiffness = 1000.0f;

        float max_force = std::numeric_limits<float>::max();

        float drive_velocity_modifier = 1.0f;

        bool enable_slerp_drive = false;

        ConstraintBlendType blend_type = ConstraintBlendType::BLEND_WITH_PREV_POSE;
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
        float partial_velocity_drive = 0.15f;

        float drive_angular_velocity_modifier = 0.4f;

        float acceleration_gain = 0.05f;
        float blend_torque_stiffness_modifier = 1.0f;

        float legs_blend_torque_stiffness_modifier = 1.0f;
        float legs_acceleration_gain = 0.05f;
    };
}