#include "physics/joint.h"
#include "physics/core/physics.h"

#include <ecs/serialization/component_registry.h>
#include <ecs/serialization/serialization_meta.h>
#include <ecs/serialization/value_adapters.h>
#include "physics/core/physics_utils.h"
#include "physics/body_component.h"

#include <rttr/registration>

namespace era_engine::physics
{

    physx::PxRevoluteJoint* create_revolute_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
    {
        auto* physics = PhysicsEngine::get_physics_core()->get_physics();

        return physx::PxRevoluteJointCreate(*physics,
            first_actor, first_actor->getGlobalPose(),
            second_actor, second_actor->getGlobalPose());
    }

    physx::PxDistanceJoint* create_distance_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
    {
        auto* physics = PhysicsEngine::get_physics_core()->get_physics();

        return physx::PxDistanceJointCreate(*physics,
            first_actor, first_actor->getGlobalPose(),
            second_actor, second_actor->getGlobalPose());
    }

    physx::PxSphericalJoint* create_spherical_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
    {
        auto* physics = PhysicsEngine::get_physics_core()->get_physics();

        return physx::PxSphericalJointCreate(*physics,
            first_actor, first_actor->getGlobalPose(),
            second_actor, second_actor->getGlobalPose());
    }

    physx::PxFixedJoint* create_fixed_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
    {
        auto* physics = PhysicsEngine::get_physics_core()->get_physics();

        return physx::PxFixedJointCreate(*physics,
            first_actor, first_actor->getGlobalPose(),
            second_actor, second_actor->getGlobalPose());
    }

    physx::PxPrismaticJoint* create_prismatic_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
    {
        auto* physics = PhysicsEngine::get_physics_core()->get_physics();

        return physx::PxPrismaticJointCreate(*physics,
            first_actor, first_actor->getGlobalPose(),
            second_actor, second_actor->getGlobalPose());
    }

    physx::PxD6Joint* create_d6_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
    {
        auto* physics = PhysicsEngine::get_physics_core()->get_physics();

        return physx::PxD6JointCreate(*physics,
            first_actor, first_actor->getGlobalPose(),
            second_actor, second_actor->getGlobalPose());
    }

    RTTR_REGISTRATION
    {
        using namespace rttr;

        registration::enumeration<JointComponent::State>("State")(
            value("ENABLED", JointComponent::State::ENABLED),
            value("DISABLED", JointComponent::State::DISABLED),
            value("BROKEN", JointComponent::State::BROKEN));

        registration::class_<JointComponent::BaseDescriptor>("JointBaseDescriptor")
            .property("connected_entity", &JointComponent::BaseDescriptor::connected_entity)
            .property("second_connected_entity", &JointComponent::BaseDescriptor::second_connected_entity)
            .property("local_frame", &JointComponent::BaseDescriptor::local_frame)
            .property("second_local_frame", &JointComponent::BaseDescriptor::second_local_frame);

        registration::class_<JointComponent>("JointComponent")
            .constructor<>()
            .property("base_descriptor", &JointComponent::get_base_descriptor, &JointComponent::set_base_descriptor)
            .property("enable_collision", &JointComponent::enable_collision)
            .property("always_update", &JointComponent::always_update)
            .property("break_force", &JointComponent::break_force)
            .property("disabled", &JointComponent::disabled)
            .property("disable_preprocessing", &JointComponent::disable_preprocessing);

        registration::class_<FixedJointComponent>("FixedJointComponent")
            .constructor<ref<Entity::EcsData>>();
        ComponentRegistry::register_component<FixedJointComponent>();

        registration::class_<RevoluteJointComponent>("RevoluteJointComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("enable_drive", &RevoluteJointComponent::enable_drive)
            .property("drive_velocity", &RevoluteJointComponent::drive_velocity)
            .property("drive_force_limit", &RevoluteJointComponent::drive_force_limit)
            .property("drive_gear_ratio", &RevoluteJointComponent::drive_gear_ratio)
            .property("linear_limit", &RevoluteJointComponent::linear_limit);
        ComponentRegistry::register_component<RevoluteJointComponent>();

        registration::class_<DistanceJointComponent>("DistanceJointComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("stiffness", &DistanceJointComponent::stiffness)
            .property("damping", &DistanceJointComponent::damping)
            .property("min_distance", &DistanceJointComponent::min_distance)
            .property("max_distance", &DistanceJointComponent::max_distance)
            .property("spring_enabled", &DistanceJointComponent::spring_enabled);
        ComponentRegistry::register_component<DistanceJointComponent>();

        registration::class_<SphericalJointComponent>("SphericalJointComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("angular_limit", &SphericalJointComponent::angular_limit);
        ComponentRegistry::register_component<SphericalJointComponent>();

        registration::enumeration<D6JointComponent::Motion>("D6JointMotion")(
            value("LOCKED", D6JointComponent::Motion::LOCKED),
            value("LIMITED", D6JointComponent::Motion::LIMITED),
            value("FREE", D6JointComponent::Motion::FREE));
        ValueAdapters::register_observable<D6JointComponent::Motion>();

        registration::class_<D6JointComponent>("D6JointComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("perform_slerp_drive", &D6JointComponent::perform_slerp_drive)
            .property("swing_y_limit", &D6JointComponent::swing_y_limit)
            .property("swing_z_limit", &D6JointComponent::swing_z_limit)
            .property("twist_min_limit", &D6JointComponent::twist_min_limit)
            .property("twist_max_limit", &D6JointComponent::twist_max_limit)
            .property("linear_limit", &D6JointComponent::linear_limit)
            .property("distance_limit", &D6JointComponent::distance_limit)
            .property("linear_x_motion_type", &D6JointComponent::linear_x_motion_type)
            .property("linear_y_motion_type", &D6JointComponent::linear_y_motion_type)
            .property("linear_z_motion_type", &D6JointComponent::linear_z_motion_type)
            .property("swing_limit_stiffness", &D6JointComponent::swing_limit_stiffness)
            .property("swing_limit_damping", &D6JointComponent::swing_limit_damping)
            .property("swing_limit_restitution", &D6JointComponent::swing_limit_restitution)
            .property("twist_limit_stiffness", &D6JointComponent::twist_limit_stiffness)
            .property("twist_limit_damping", &D6JointComponent::twist_limit_damping)
            .property("twist_limit_restitution", &D6JointComponent::twist_limit_restitution)
            .property("swing_y_motion_type", &D6JointComponent::swing_y_motion_type)
            .property("swing_z_motion_type", &D6JointComponent::swing_z_motion_type)
            .property("twist_motion_type", &D6JointComponent::twist_motion_type)
            .property("slerp_drive_force_limit", &D6JointComponent::slerp_drive_force_limit)
            .property("swing_drive_force_limit", &D6JointComponent::swing_drive_force_limit)
            .property("twist_drive_force_limit", &D6JointComponent::twist_drive_force_limit)
            .property("linear_drive_force_limit", &D6JointComponent::linear_drive_force_limit)
            .property("drive_limits_are_forces", &D6JointComponent::drive_limits_are_forces)
            .property("improved_slerp", &D6JointComponent::improved_slerp)
            .property("slerp_drive_stiffness", &D6JointComponent::slerp_drive_stiffness)
            .property("swing_drive_stiffness", &D6JointComponent::swing_drive_stiffness)
            .property("twist_drive_stiffness", &D6JointComponent::twist_drive_stiffness)
            .property("linear_drive_stiffness", &D6JointComponent::linear_drive_stiffness)
            .property("slerp_drive_damping", &D6JointComponent::slerp_drive_damping)
            .property("swing_drive_damping", &D6JointComponent::swing_drive_damping)
            .property("twist_drive_damping", &D6JointComponent::twist_drive_damping)
            .property("linear_drive_damping", &D6JointComponent::linear_drive_damping)
            .property("slerp_drive_accelerated", &D6JointComponent::slerp_drive_accelerated)
            .property("swing_drive_accelerated", &D6JointComponent::swing_drive_accelerated)
            .property("twist_drive_accelerated", &D6JointComponent::twist_drive_accelerated)
            .property("linear_drive_accelerated", &D6JointComponent::linear_drive_accelerated)
            .property("angular_drive_velocity", &D6JointComponent::angular_drive_velocity)
            .property("linear_drive_velocity", &D6JointComponent::linear_drive_velocity)
            .property("drive_transform", &D6JointComponent::drive_transform);
        ComponentRegistry::register_component<D6JointComponent>();
    }

    JointComponent::JointComponent(ref<Entity::EcsData> _data, const BaseDescriptor& _base_descriptor)
        : Component(_data), base_descriptor(_base_descriptor)
    {
        enable_collision.set_component(ComponentPtr(this));
        break_force.set_component(ComponentPtr(this));
        always_update.set_component(ComponentPtr(this));
        disabled.set_component(ComponentPtr(this));
        disable_preprocessing.set_component(ComponentPtr(this));
    }

    JointComponent::~JointComponent()
    {
    }

    const JointComponent::BaseDescriptor& JointComponent::get_base_descriptor() const
    {
        return base_descriptor;
    }

    void JointComponent::set_base_descriptor(const BaseDescriptor& new_base_descriptor)
    {
        base_descriptor = new_base_descriptor;
    }

    JointComponent::State JointComponent::get_state() const
    {
        return state;
    }

    physx::PxJoint* JointComponent::get_native_joint() const
    {
        return joint;
    }

    EntityPtr JointComponent::get_first_entity_ptr() const
    {
        return base_descriptor.connected_entity;
    }

    EntityPtr JointComponent::get_second_entity_ptr() const
    {
        return base_descriptor.second_connected_entity;
    }

    const trs& JointComponent::get_first_local_frame() const
    {
        return base_descriptor.local_frame;
    }

    const trs& JointComponent::get_second_local_frame() const
    {
        return base_descriptor.second_local_frame;
    }

    FixedJointComponent::FixedJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
        : JointComponent(_data, _base_descriptor)
    {
    }

    FixedJointComponent::~FixedJointComponent()
    {
    }

    RevoluteJointComponent::RevoluteJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
        : JointComponent(_data, _base_descriptor)
    {
        drive_force_limit.set_component(ComponentPtr(this));
        drive_gear_ratio.set_component(ComponentPtr(this));
        drive_velocity.set_component(ComponentPtr(this));
        linear_limit.set_component(ComponentPtr(this));
    }

    RevoluteJointComponent::~RevoluteJointComponent()
    {
    }

    DistanceJointComponent::DistanceJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
        : JointComponent(_data, _base_descriptor)
    {
        stiffness.set_component(ComponentPtr(this));
        damping.set_component(ComponentPtr(this));
        spring_enabled.set_component(ComponentPtr(this));
        max_distance.set_component(ComponentPtr(this));
        min_distance.set_component(ComponentPtr(this));
    }

    DistanceJointComponent::~DistanceJointComponent()
    {
    }

    SphericalJointComponent::SphericalJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
        : JointComponent(_data, _base_descriptor)
    {
        angular_limit.set_component(ComponentPtr(this));
    }

    SphericalJointComponent::~SphericalJointComponent()
    {
    }

    D6JointComponent::D6JointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
        : JointComponent(_data, _base_descriptor)
    {
        drive_transform.set_component(ComponentPtr(this));
        linear_drive_velocity.set_component(ComponentPtr(this));
        angular_drive_velocity.set_component(ComponentPtr(this));

        linear_drive_accelerated.set_component(ComponentPtr(this));
        twist_drive_accelerated.set_component(ComponentPtr(this));
        swing_drive_accelerated.set_component(ComponentPtr(this));
        slerp_drive_accelerated.set_component(ComponentPtr(this));

        linear_drive_stiffness.set_component(ComponentPtr(this));
        twist_drive_stiffness.set_component(ComponentPtr(this));
        swing_drive_stiffness.set_component(ComponentPtr(this));
        slerp_drive_stiffness.set_component(ComponentPtr(this));

        linear_drive_force_limit.set_component(ComponentPtr(this));
        twist_drive_force_limit.set_component(ComponentPtr(this));
        swing_drive_force_limit.set_component(ComponentPtr(this));
        slerp_drive_force_limit.set_component(ComponentPtr(this));

        twist_motion_type.set_component(ComponentPtr(this));
        swing_z_motion_type.set_component(ComponentPtr(this));
        swing_y_motion_type.set_component(ComponentPtr(this));

        linear_x_motion_type.set_component(ComponentPtr(this));
        linear_y_motion_type.set_component(ComponentPtr(this));
        linear_z_motion_type.set_component(ComponentPtr(this));

        swing_z_limit.set_component(ComponentPtr(this));
        swing_y_limit.set_component(ComponentPtr(this));

        twist_max_limit.set_component(ComponentPtr(this));
        twist_min_limit.set_component(ComponentPtr(this));
        linear_z_motion_type.set_component(ComponentPtr(this));
        linear_z_motion_type.set_component(ComponentPtr(this));

        linear_limit.set_component(ComponentPtr(this));
        distance_limit.set_component(ComponentPtr(this));

        drive_limits_are_forces.set_component(ComponentPtr(this));

        improved_slerp.set_component(ComponentPtr(this));

        swing_limit_damping.set_component(ComponentPtr(this));
        swing_limit_stiffness.set_component(ComponentPtr(this));
        swing_limit_restitution.set_component(ComponentPtr(this));

        twist_limit_damping.set_component(ComponentPtr(this));
        twist_limit_stiffness.set_component(ComponentPtr(this));
        twist_limit_restitution.set_component(ComponentPtr(this));
    }

    D6JointComponent::~D6JointComponent()
    {
    }

}