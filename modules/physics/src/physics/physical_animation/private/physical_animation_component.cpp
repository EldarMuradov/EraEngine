#include "physics/physical_animation/physical_animation_component.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/physical_animation/states/blend_in_simulation_state.h"
#include "physics/physical_animation/states/blend_out_simulation_state.h"
#include "physics/physical_animation/states/enabled_simulation_state.h"
#include "physics/physical_animation/states/disabled_simulation_state.h"
#include "physics/physical_animation/drive_pose_solver.h"
#include "physics/physical_animation/limb_states/blend_out_limb_state.h"
#include "physics/physical_animation/limb_states/kinematic_limb_state.h"
#include "physics/physical_animation/limb_states/simulation_limb_state.h"

#include <rttr/registration>

namespace era_engine::physics
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<PhysicalAnimationComponent>("PhysicalAnimationComponent")
			.constructor<>();

		registration::class_<PhysicalAnimationLimbComponent>("PhysicalAnimationLimbComponent")
			.constructor<>();
    }

    bool PhysicsLimbChain::has_any_blocked_limb() const
    {
        for (EntityPtr limb_ptr : connected_limbs)
        {
            Entity limb = limb_ptr.get();

            const PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();
            ASSERT(limb_component != nullptr);

            if (limb_component->is_blocked || limb_component->blocked_blend_factor > 0.0f)
            {
                return true;
            }
        }
        return false;
    }

    bool PhysicsLimbChain::has_any_colliding_limb() const
    {
        for (EntityPtr limb_ptr : connected_limbs)
        {
            Entity limb = limb_ptr.get();

            const PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();
            ASSERT(limb_component != nullptr);

            if (limb_component->is_colliding || limb_component->collision_time > 0.0f)
            {
                return true;
            }
        }
        return false;
    }

    float PhysicalAnimationLimbComponent::calculate_desired_damping(float delta_angle) const
    {
        float angular_damping = damping;
        if (dynamic_update_damping)
        {
            angular_damping = map_value(delta_angle,
                angle_range.x,
                angle_range.y,
                damping_range.x,
                damping_range.y);
        }

        return angular_damping;
    }

	PhysicalAnimationLimbComponent::PhysicalAnimationLimbComponent(ref<Entity::EcsData> _data, uint32 _joint_id /*= INVALID_JOINT*/)
		: RagdollLimbComponent(_data, _joint_id)
	{
        ComponentPtr this_component_ptr = ComponentPtr(this);

        simulation_states.emplace(ConstraintLimbStateType::KINEMATIC, std::make_shared<KinematicLimbState>(this_component_ptr));
        simulation_states.emplace(ConstraintLimbStateType::TRANSITION, std::make_shared<BlendOutLimbState>(this_component_ptr));
        simulation_states.emplace(ConstraintLimbStateType::SIMULATION, std::make_shared<SimulationLimbState>(this_component_ptr));
	}

	PhysicalAnimationLimbComponent::~PhysicalAnimationLimbComponent()
	{
	}

    std::shared_ptr<BaseLimbState> PhysicalAnimationLimbComponent::get_current_state() const
    {
        return simulation_states.at(current_state_type);
    }

    ConstraintLimbStateType PhysicalAnimationLimbComponent::get_current_state_type() const
    {
        return current_state_type;
    }

    void PhysicalAnimationLimbComponent::force_switch_state(ConstraintLimbStateType desired_state)
    {
        get_current_state()->on_exit();
        current_state_type = desired_state;
        get_current_state()->on_enter();
    }

    void PhysicalAnimationLimbComponent::update_states(float dt, ConstraintLimbStateType desired_state)
    {
        if (current_state_type != desired_state)
        {
            const ConstraintLimbStateType transition_state = get_current_state()->try_switch_to(desired_state);
            if (current_state_type != transition_state)
            {
                force_switch_state(transition_state);
            }
        }

        get_current_state()->update(dt);
    }

	PhysicalAnimationComponent::PhysicalAnimationComponent(ref<Entity::EcsData> _data)
		: RagdollComponent(_data)
	{
        ComponentPtr this_component_ptr = ComponentPtr(this);

        simulation_states.emplace(SimulationStateType::ENABLED, std::make_shared<EnabledSimulationState>(this_component_ptr));
        simulation_states.emplace(SimulationStateType::DISABLED, std::make_shared<DisabledSimulationState>(this_component_ptr));
        simulation_states.emplace(SimulationStateType::BLEND_IN, std::make_shared<BlendInSimulationState>(this_component_ptr));
        simulation_states.emplace(SimulationStateType::BLEND_OUT, std::make_shared<BlendOutSimulationState>(this_component_ptr));

        current_state_type = SimulationStateType::DISABLED;

        pose_solver = std::make_unique<DrivePoseSolver>(this_component_ptr);
	}

	PhysicalAnimationComponent::~PhysicalAnimationComponent()
	{
	}

	bool PhysicalAnimationComponent::try_to_apply_ragdoll_profile(ref<RagdollProfile> new_profile, bool force_reload)
	{
        if (current_profile != new_profile || force_reload)
        {
            current_profile = new_profile;

            for (const EntityPtr& limb_ptr : limbs)
            {
                Entity limb = limb_ptr.get();
                PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();

                const ConstraintDetails* details = nullptr;

                switch (limb_component->type)
                {
                case RagdollLimbComponent::Type::HEAD:
                {
                    details = &current_profile->head_constraint;
                }
                break;

                case RagdollLimbComponent::Type::NECK:
                {
                    details = &current_profile->neck_constraint;
                }
                break;

                case RagdollLimbComponent::Type::BODY_UPPER:
                {
                    details = &current_profile->body_upper_constraint;
                }
                break;

                case RagdollLimbComponent::Type::BODY_LOWER:
                case RagdollLimbComponent::Type::BODY_MIDDLE:
                {
                    details = &current_profile->body_middle_constraint;
                }
                break;

                case RagdollLimbComponent::Type::ARM:
                {
                    details = &current_profile->arm_constraint;
                }
                break;

                case RagdollLimbComponent::Type::FOREARM:
                {
                    details = &current_profile->forearm_constraint;
                }
                break;

                case RagdollLimbComponent::Type::HAND:
                {
                    details = &current_profile->hand_constraint;
                }
                break;

                case RagdollLimbComponent::Type::LEG:
                {
                    details = &current_profile->leg_constraint;
                }
                break;

                case RagdollLimbComponent::Type::CALF:
                {
                    details = &current_profile->calf_constraint;
                }
                break;

                case RagdollLimbComponent::Type::FOOT:
                {
                    details = &current_profile->foot_constraint;
                }
                break;

                default:
                {
                    details = &current_profile->body_upper_constraint;
                }
                break;
                }

                if (details != nullptr)
                {
                    limb_component->stiffness = details->drive_stiffness;
                    limb_component->damping = details->drive_damping;
                    limb_component->drive_velocity_modifier = details->drive_velocity_modifier;

                    limb_component->blend_type = details->blend_type;

                    D6JointComponent* joint_component = dynamic_cast<D6JointComponent*>(limb_component->parent_joint_component.get_for_write());
                    if (joint_component)
                    {
                        if (details->enable_slerp_drive)
                        {
                            joint_component->slerp_drive_damping = details->drive_damping;
                            joint_component->slerp_drive_force_limit = details->max_force;
                            joint_component->slerp_drive_stiffness = details->drive_stiffness;
                        }
                        else
                        {
                            joint_component->swing_drive_damping = details->drive_damping;
                            joint_component->swing_drive_force_limit = details->max_force;
                            joint_component->swing_drive_stiffness = details->drive_stiffness;

                            joint_component->twist_drive_damping = details->drive_damping;
                            joint_component->twist_drive_force_limit = details->max_force;
                            joint_component->twist_drive_stiffness = details->drive_stiffness;
                        }
                    }
                }
            }

            // If we update ragdoll profile, we need to combine prev and current blend types untill ragdoll_profile_transition_time is over.
            ragdoll_profile_transition_time = max_ragdoll_profile_transition_time;

            return true;
        }
        return false;
    }

    float PhysicalAnimationComponent::get_profile_transition_time() const
    {
        return ragdoll_profile_transition_time;
    }

    bool PhysicalAnimationComponent::is_in_transition() const
    {
        return ragdoll_profile_transition_time > 0.0f;
    }

    void PhysicalAnimationComponent::update_profile_transition(float dt)
    {
        if (fuzzy_equals(ragdoll_profile_transition_time, 0.0f))
        {
            return;
        }

        if (ragdoll_profile_transition_time > 0.0f)
        {
            ragdoll_profile_transition_time -= dt;
        }

        if (ragdoll_profile_transition_time < 0.0f)
        {
            ragdoll_profile_transition_time = 0.0f;

            for (auto& limb_ptr : limbs)
            {
                PhysicalAnimationLimbComponent* limb_component = limb_ptr.get().get_component<PhysicalAnimationLimbComponent>();
                ASSERT(limb_component != nullptr);

                limb_component->prev_blend_type = ConstraintBlendType::NONE;
            }
        }
    }

	ref<RagdollProfile> PhysicalAnimationComponent::get_ragdoll_profile() const
	{
		return current_profile;
	}

    std::shared_ptr<BaseSimulationState> PhysicalAnimationComponent::get_current_state() const
    {
        return simulation_states.at(current_state_type);
    }

    SimulationStateType PhysicalAnimationComponent::get_current_state_type() const
    {
        return current_state_type;
    }

    void PhysicalAnimationComponent::force_switch_state(SimulationStateType desired_state)
    {
        current_state_type = desired_state;
        get_current_state()->on_entered();
    }

    bool PhysicalAnimationComponent::is_in_idle() const
    {
        return current_profile->type == RagdollProfileType::IDLE;
    }

    void PhysicalAnimationComponent::update_states(float dt, SimulationStateType desired_state)
    {
        if (current_state_type != desired_state)
        {
            const SimulationStateType transition_state = get_current_state()->try_switch_to(desired_state);
            if (current_state_type != transition_state)
            {
                force_switch_state(transition_state);
            }
        }

        get_current_state()->update(dt);

        update_profile_transition(dt);
    }

}