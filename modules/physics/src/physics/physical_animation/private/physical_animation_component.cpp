#include "physics/physical_animation/physical_animation_component.h"
#include "physics/physical_animation/dismemberment/ragdoll_dismemberment_component.h"
#include "physics/body_component.h"
#include "physics/joint.h"

#include <ecs/serialization/component_registry.h>
#include <ecs/serialization/serialization_meta.h>
#include "physics/physical_animation/states/blend_in_simulation_state.h"
#include "physics/physical_animation/states/blend_out_simulation_state.h"
#include "physics/physical_animation/states/enabled_simulation_state.h"
#include "physics/physical_animation/states/disabled_simulation_state.h"
#include "physics/physical_animation/states/ragdoll_simulation_state.h"
#include "physics/physical_animation/limb_states/blend_out_limb_state.h"
#include "physics/physical_animation/limb_states/kinematic_limb_state.h"
#include "physics/physical_animation/limb_states/simulation_limb_state.h"
#include "physics/physical_animation/limb_states/ragdoll_limb_state.h"
#include "physics/physical_animation/dismemberment/dismembered_limb_state.h"

#include <rttr/registration>

#include <algorithm>

namespace era_engine::physics
{

    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<PhysicalAnimationComponent>("PhysicalAnimationComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("use_fixed_pelvis_attachment", &PhysicalAnimationComponent::use_fixed_pelvis_attachment)
            .property("blend_in_time", &PhysicalAnimationComponent::blend_in_time)
            .property("blend_out_time", &PhysicalAnimationComponent::blend_out_time)
            .property("animation_blend_factor", &PhysicalAnimationComponent::animation_blend_factor);
        ComponentRegistry::register_component<PhysicalAnimationComponent>();

        registration::class_<PhysicalAnimationLimbComponent>("PhysicalAnimationLimbComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<>();
        ComponentRegistry::register_component<PhysicalAnimationLimbComponent>();
    }

    bool PhysicsLimbChain::has_any_colliding_limb() const
    {
        for (EntityPtr limb_ptr : connected_limbs)
        {
            Entity limb = limb_ptr.get();

            const PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();
            ASSERT(limb_component != nullptr);

            if (limb_component->collision.is_colliding || limb_component->collision.collision_time > 0.0f)
            {
                return true;
            }
        }
        return false;
    }

    bool PhysicsLimbChain::has_any_dismembered_limb() const
    {
        for (EntityPtr limb_ptr : connected_limbs)
        {
            Entity limb = limb_ptr.get();

            const PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();
            ASSERT(limb_component != nullptr);

            if (limb_component->get_current_state_type() == PhysicalLimbStateType::DISMEMBERED)
            {
                return true;
            }
        }
        return false;
    }

    float PhysicalAnimationLimbComponent::calculate_desired_angular_damping(float delta_angle, float strength_multiplier /*= 1.0f*/) const
    {
        const float angular_damping_value = map_value(delta_angle,
            drive.angular.error_range.x,
            drive.angular.error_range.y,
            drive.angular.damping_range.x,
            drive.angular.damping_range.y);

        return angular_damping_value * strength_multiplier;
    }

    float PhysicalAnimationLimbComponent::calculate_desired_linear_damping(float delta_position, float strength_multiplier /*= 1.0f*/) const
    {
        const float linear_damping_value = map_value(delta_position,
            drive.linear.error_range.x,
            drive.linear.error_range.y,
            drive.linear.damping_range.x,
            drive.linear.damping_range.y);

        return linear_damping_value * strength_multiplier;
    }

    void PhysicalAnimationLimbComponent::apply_impulse(const vec3& impulse, const vec3& local_point /* = vec3::zero*/)
    {
        DynamicBodyComponent* dynamic_body_component = get_entity().get_component<DynamicBodyComponent>();
        Force& force = dynamic_body_component->forces.emplace_back();
        force.force = impulse;
        force.point = local_point;
        force.mode = ForceMode::IMPULSE;

        collision.impulse += impulse;
    }

    void PhysicalAnimationLimbComponent::CollisionData::reset()
    {
        was_in_collision = false;
        is_colliding = false;
        collision_time = 0.0f;
        impulse = vec3::zero;
    }

    PhysicalAnimationLimbComponent::PhysicalAnimationLimbComponent(ref<Entity::EcsData> _data, uint32 _joint_id /*= INVALID_JOINT*/)
        : RagdollLimbComponent(_data, _joint_id)
    {
        ComponentPtr this_component_ptr = ComponentPtr(this);

        simulation_states.emplace(PhysicalLimbStateType::KINEMATIC, std::make_shared<KinematicLimbState>(this_component_ptr));
        simulation_states.emplace(PhysicalLimbStateType::BLEND_OUT, std::make_shared<BlendOutLimbState>(this_component_ptr));
        simulation_states.emplace(PhysicalLimbStateType::SIMULATION, std::make_shared<SimulationLimbState>(this_component_ptr));
        simulation_states.emplace(PhysicalLimbStateType::RAGDOLL, std::make_shared<RagdollLimbState>(this_component_ptr));
        simulation_states.emplace(PhysicalLimbStateType::PARTIAL_DISMEMBERED, std::make_shared<PartialDismemberedLimbState>(this_component_ptr));
        simulation_states.emplace(PhysicalLimbStateType::DISMEMBERED, std::make_shared<DismemberedLimbState>(this_component_ptr));
    }

    PhysicalAnimationLimbComponent::~PhysicalAnimationLimbComponent()
    {
    }

    std::shared_ptr<BaseLimbState> PhysicalAnimationLimbComponent::get_current_state() const
    {
        return simulation_states.at(current_state_type);
    }

    PhysicalLimbStateType PhysicalAnimationLimbComponent::get_current_state_type() const
    {
        return current_state_type;
    }

    void PhysicalAnimationLimbComponent::force_switch_state(PhysicalLimbStateType desired_state)
    {
        if (current_state_type != desired_state)
        {
            get_current_state()->on_exit();
            current_state_type = desired_state;
            get_current_state()->on_enter();
        }
    }

    void PhysicalAnimationLimbComponent::update_states(float dt, PhysicalLimbStateType desired_state)
    {
        if (current_state_type != desired_state)
        {
            const PhysicalLimbStateType transition_state = get_current_state()->try_switch_to(desired_state);

            force_switch_state(transition_state);
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
        simulation_states.emplace(SimulationStateType::RAGDOLL, std::make_shared<RagdollSimulationState>(this_component_ptr));

        current_state_type = SimulationStateType::DISABLED;

        neck_chain = make_ref<PhysicsLimbChain>();
        body_chain = make_ref<PhysicsLimbChain>();

        left_arm_chain = make_ref<PhysicsLimbChain>();
        right_arm_chain = make_ref<PhysicsLimbChain>();

        left_leg_chain = make_ref<PhysicsLimbChain>();
        right_leg_chain = make_ref<PhysicsLimbChain>();
    }

    PhysicalAnimationComponent::~PhysicalAnimationComponent()
    {
    }

    bool PhysicalAnimationComponent::try_to_apply_ragdoll_profile(ref<RagdollProfile> new_profile, RagdollProfileStrengthType new_strength_type /* = RagdollProfileStrengthType::DEFAULT*/, bool force_reload)
    {
        if (current_profile == new_profile &&
            strength_type == new_strength_type &&
            !force_reload)
        {
            return false;
        }

        current_profile = new_profile;
        strength_type = new_strength_type;

        for (const EntityPtr& limb_ptr : limbs)
        {
            PhysicalAnimationLimbComponent* limb_component = limb_ptr.get().get_component<PhysicalAnimationLimbComponent>();
            limb_component->blend_type = current_profile->get_limb_details_by_type(limb_component->type).blend_type;
        }

        drives_dirty = true;

        ragdoll_profile_transition_time = MAX_RAGDOLL_PROFILE_TRANSITION_TIME;

        return true;
    }

    RagdollProfileStrengthType PhysicalAnimationComponent::get_strength_type() const
    {
        return strength_type;
    }

    void PhysicalAnimationComponent::update_drives(float dt)
    {
        if (!loaded || current_profile == nullptr)
        {
            return;
        }

        // Mass properties are measured once.
        if (!mass_properties_initialized)
        {
            for (const EntityPtr& limb_ptr : limbs)
            {
                PhysicalAnimationLimbComponent* limb_component = limb_ptr.get().get_component<PhysicalAnimationLimbComponent>();
                limb_component->mass_properties = RagdollDriveCompiler::compute_limb_mass_properties(limb_component);
            }
            mass_properties_initialized = true;
            drives_dirty = true;
        }

        const uint32 asset_revision = profiles_asset != nullptr ? profiles_asset->revision : 0u;
        if (applied_asset_revision != asset_revision)
        {
            applied_asset_revision = asset_revision;
            drives_dirty = true;
        }

        if (drives_dirty)
        {
            const bool first_compilation = !drives_compiled;

            for (const EntityPtr& limb_ptr : limbs)
            {
                PhysicalAnimationLimbComponent* limb_component = limb_ptr.get().get_component<PhysicalAnimationLimbComponent>();

                const PhysicalLimbDetails& limb_details = current_profile->get_limb_details_by_type(limb_component->type);
                if (!limb_details.motor_drive.has_value() || limb_component->drive_joint_component.is_empty())
                {
                    // Profiles without drives keep the current values.
                    limb_component->drive_transition_from = limb_component->drive;
                    limb_component->drive_transition_to = limb_component->drive;
                    continue;
                }

                const float strength = current_profile->get_strength(limb_component->type, strength_type);

                limb_component->drive_transition_from = first_compilation ? CompiledLimbDrive{} : limb_component->drive;
                limb_component->drive_transition_to = RagdollDriveCompiler::compile_limb_drive(limb_details.motor_drive.value(), limb_component->mass_properties, strength);
            }

            drive_transition_elapsed = 0.0f;
            drive_transition_duration = first_compilation || profiles_asset == nullptr ? 0.0f : profiles_asset->selection_settings.transition_time;

            drives_compiled = true;
            drives_dirty = false;
        }

        drive_transition_elapsed = std::min(drive_transition_elapsed + dt, drive_transition_duration);
        const float blend = drive_transition_duration > 0.0f ? drive_transition_elapsed / drive_transition_duration : 1.0f;

        for (const EntityPtr& limb_ptr : limbs)
        {
            PhysicalAnimationLimbComponent* limb_component = limb_ptr.get().get_component<PhysicalAnimationLimbComponent>();

            D6JointComponent* joint_component = dynamic_cast<D6JointComponent*>(limb_component->drive_joint_component.get_for_write());
            if (joint_component == nullptr)
            {
                continue;
            }

            limb_component->drive = CompiledLimbDrive::lerp(limb_component->drive_transition_from, limb_component->drive_transition_to, blend);

            joint_component->slerp_drive_stiffness = limb_component->drive.angular.stiffness;
            joint_component->slerp_drive_force_limit = limb_component->drive.angular.force_limit;
            joint_component->slerp_drive_accelerated = limb_component->drive.angular.accelerated;

            joint_component->linear_drive_stiffness = limb_component->drive.linear.stiffness;
            joint_component->linear_drive_force_limit = limb_component->drive.linear.force_limit;
            joint_component->linear_drive_accelerated = limb_component->drive.linear.accelerated;
        }
    }

    void PhysicalAnimationComponent::push_animation_pose(const animation::SkeletonPose& pose, float dt)
    {
        if (!pose.is_valid())
        {
            return;
        }

        animation_elapsed += dt;

        animation_pose_snapshots[0] = std::move(animation_pose_snapshots[1]);
        animation_pose_snapshots[1].pose = pose;
        animation_pose_snapshots[1].time = animation_elapsed;

        animation_pose_snapshots_count = std::min(animation_pose_snapshots_count + 1u, 2u);
    }

    bool PhysicalAnimationComponent::sample_animation_pose(float dt, animation::SkeletonPose& out_pose)
    {
        using namespace animation;

        if (animation_pose_snapshots_count == 0)
        {
            return false;
        }

        const AnimationPoseSnapshot& latest = animation_pose_snapshots[1];

        if (animation_pose_snapshots_count == 1 || animation_pose_snapshots[0].pose.size() != latest.pose.size())
        {
            physics_elapsed = latest.time;
            out_pose = latest.pose;
            return true;
        }

        const AnimationPoseSnapshot& previous = animation_pose_snapshots[0];

        // Fixed steps of a frame run before its animation update, so the physics elapsed walks over the interval between
        // the two latest animation frames. Clamping keeps it there if steps were dropped or the frame rate changed.
        physics_elapsed = clamp(physics_elapsed + dt, previous.time, latest.time);

        const float interval = latest.time - previous.time;
        const float t = interval > 0.0f ? (physics_elapsed - previous.time) / interval : 1.0f;

        out_pose = latest.pose;
        for (uint32 joint_id = 0; joint_id < latest.pose.size(); ++joint_id)
        {
            const trs& from = previous.pose.get_joint_transform(joint_id).get_transform();
            const trs& to = latest.pose.get_joint_transform(joint_id).get_transform();

            trs joint_transform;
            joint_transform.position = lerp(from.position, to.position, t);
            joint_transform.rotation = nlerp_shortest(from.rotation, to.rotation, t);
            joint_transform.scale = lerp(from.scale, to.scale, t);

            out_pose.set_joint_transform(JointTransform(joint_transform), joint_id);
        }

        return true;
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

                limb_component->prev_blend_type = PhysicalLimbBlendType::NONE;
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

    void PhysicalAnimationComponent::reset_blends()
    {
        blend_weight = 0.0f;
        reached_physics_pose = false;
        elapsed_blend_time = 0.0f;
    }

    SimulationStateType PhysicalAnimationComponent::get_current_state_type() const
    {
        return current_state_type;
    }

    void PhysicalAnimationComponent::force_switch_state(SimulationStateType desired_state)
    {
        if (current_state_type != desired_state)
        {
            get_current_state()->on_exit();
            current_state_type = desired_state;
            get_current_state()->on_enter();
        }
    }

    bool PhysicalAnimationComponent::is_in_idle() const
    {
        return current_profile->type == RagdollProfileType::IDLE;
    }

    bool PhysicalAnimationComponent::is_in_ragdoll() const
    {
        return force_ragdoll_state;
    }

    void PhysicalAnimationComponent::force_set_ragdoll(bool enabled)
    {
        force_ragdoll_state = enabled;
    }

    void PhysicalAnimationComponent::update_states(float dt, SimulationStateType desired_state)
    {
        if (current_state_type != desired_state)
        {
            const SimulationStateType transition_state = get_current_state()->try_switch_to(desired_state);

            force_switch_state(transition_state);
        }

        get_current_state()->update(dt);

        update_profile_transition(dt);
    }

    float PhysicalAnimationComponent::get_strength_value_by_limb(RagdollLimbType limb_type) const
    {
        return current_profile->get_strength(limb_type, strength_type);
    }

    const PhysicsLimbChain* PhysicalAnimationComponent::get_chain_by_joint_id(uint32 joint_id) const
    {
        auto contains_joint = [this, joint_id](const ref<PhysicsLimbChain>& chain) -> bool
        {
            for (EntityPtr limb_ptr : chain->connected_limbs)
            {
                const PhysicalAnimationLimbComponent* limb_component = limb_ptr.get().get_component<PhysicalAnimationLimbComponent>();

                if (limb_component->joint_id == joint_id)
                {
                    return true;
                }
            }

            return false;
        };

        if (contains_joint(body_chain))
        {
            return body_chain.get();
        }

        if (contains_joint(neck_chain))
        {
            return neck_chain.get();
        }

        if (contains_joint(left_arm_chain))
        {
            return left_arm_chain.get();
        }

        if (contains_joint(right_arm_chain))
        {
            return right_arm_chain.get();
        }

        if (contains_joint(left_leg_chain))
        {
            return left_leg_chain.get();
        }

        if (contains_joint(right_leg_chain))
        {
            return right_leg_chain.get();
        }

        return nullptr;
    }

}