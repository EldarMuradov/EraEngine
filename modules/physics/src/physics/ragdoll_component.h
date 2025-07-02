#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include <core/math.h>

#include <animation/animation.h>

#include <ecs/component.h>
#include <ecs/observable_member.h>

namespace era_engine::physics
{
    struct ERA_PHYSICS_API RagdollJointIds
    {
        uint32_t head_end_idx = INVALID_JOINT;
        uint32_t head_idx = INVALID_JOINT;
        uint32_t neck_idx = INVALID_JOINT;
        uint32_t spine_03_idx = INVALID_JOINT;
        uint32_t spine_02_idx = INVALID_JOINT;
        uint32_t spine_01_idx = INVALID_JOINT;
        uint32_t pelvis_idx = INVALID_JOINT;

        uint32_t root_idx = INVALID_JOINT;

        uint32_t thigh_l_idx = INVALID_JOINT;
        uint32_t calf_l_idx = INVALID_JOINT;
        uint32_t foot_l_idx = INVALID_JOINT;
        uint32_t foot_end_l_idx = INVALID_JOINT;

        uint32_t thigh_r_idx = INVALID_JOINT;
        uint32_t calf_r_idx = INVALID_JOINT;
        uint32_t foot_r_idx = INVALID_JOINT;
        uint32_t foot_end_r_idx = INVALID_JOINT;

        uint32_t upperarm_l_idx = INVALID_JOINT;
        uint32_t lowerarm_l_idx = INVALID_JOINT;
        uint32_t hand_l_idx = INVALID_JOINT;
        uint32_t hand_end_l_idx = INVALID_JOINT;

        uint32_t upperarm_r_idx = INVALID_JOINT;
        uint32_t lowerarm_r_idx = INVALID_JOINT;
        uint32_t hand_r_idx = INVALID_JOINT;
        uint32_t hand_end_r_idx = INVALID_JOINT;
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

        float slerp_drive_damping = 1000.0f;
        float slerp_drive_stiffness = 0.0f;

        float max_force = std::numeric_limits<float>::max();
    };

    class ERA_PHYSICS_API RagdollSettings final
    {
    public:
        float head_mass_percentage = 0.0826f;
        float body_upper_mass_percentage = 0.204f;
        float body_lower_mass_percentage = 0.204f;
        float arm_mass_percentage = 0.07f;
        float forearm_mass_percentage = 0.0467f;
        float hand_mass_percentage = 0.015f;
        float up_leg_mass_percentage = 0.085f;
        float leg_mass_percentage = 0.0475f;
        float foot_mass_percentage = 0.024f;

        float max_head_contact_impulse = 220.0f;
        float max_hand_contact_impulse = 60.0f;
        float max_forearm_contact_impulse = 60.0f;
        float max_arm_contact_impulse = 100.0f;
        float max_body_contact_impulse = 200.0f;
        float max_up_leg_contact_impulse = 150.0f;
        float max_leg_contact_impulse = 150.0f;
        float max_foot_contact_impulse = 40.0f;

        float arm_radius = 0.081f;
        float forearm_radius = 0.063f;
        float hand_width = 0.08f;
        float hand_height = 0.04f;
        float up_leg_radius = 0.09f;
        float leg_radius = 0.06f;
        float foot_width = 0.1f;
        float neck_radius = 0.1f;
        float neck_half_height = 0.03f;
        float head_radius = 0.1f;
        float head_half_height = 0.04f;

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

        vec3 head_joint_adjastment = vec3::zero;
        vec3 head_end_joint_adjastment = vec3::zero;
        vec3 neck_joint_adjastment = vec3::zero;
        vec3 thorax_joint_adjastment = vec3::zero;
        vec3 abdomen_joint_adjastment = vec3::zero;
        vec3 pelvis_joint_adjastment = vec3::zero;

        float upper_body_height_modifier = 1.0f;
        float lower_body_height_modifier = 1.0f;

        float upper_body_radius_modifier = 1.0f;
        float lower_body_radius_modifier = 1.0f;
    };

	class ERA_PHYSICS_API RagdollLimbComponent : public Component
	{
	public:
		RagdollLimbComponent() = default;
		RagdollLimbComponent(ref<Entity::EcsData> _data, uint32 _joint_id = INVALID_JOINT);

		~RagdollLimbComponent() override;

		uint32 joint_id = INVALID_JOINT;

        vec3 prev_limb_local_position = vec3::zero;
        quat prev_limb_local_rotation = quat::identity;

        ObservableMember<bool> simulated = false;

        EntityPtr joint_entity_ptr;

        ERA_VIRTUAL_REFLECT(Component)
	};

	class ERA_PHYSICS_API RagdollComponent : public Component
	{
	public:
		RagdollComponent() = default;
		RagdollComponent(ref<Entity::EcsData> _data);
		~RagdollComponent() override;

		ObservableMember<bool> simulated = false;

        RagdollSettings settings;
        RagdollJointIds joint_init_ids;

        float mass = 100.0f; // Set before creation.

        float blend_factor = 0.5f;

		std::vector<EntityPtr> limbs;

        ERA_VIRTUAL_REFLECT(Component)
	};
}