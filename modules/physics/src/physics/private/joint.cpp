#include "physics/joint.h"
#include "physics/core/physics.h"
#include "physics/core/physics_utils.h"
#include "physics/body_component.h"

#include <rttr/registration>

namespace era_engine::physics
{

	physx::PxRevoluteJoint* create_revolute_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxRevoluteJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxDistanceJoint* create_distance_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxDistanceJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxSphericalJoint* create_spherical_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxSphericalJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxFixedJoint* create_fixed_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxFixedJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxPrismaticJoint* create_prismatic_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxPrismaticJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxD6Joint* create_d6_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxD6JointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::enumeration<JointComponent::JointState>("JointState")
		(
			value("ENABLED", JointComponent::ENABLED),
			value("DISABLED", JointComponent::DISABLED),
			value("BROKEN", JointComponent::BROKEN)
		);

		rttr::registration::class_<JointComponent>("JointComponent")
			.constructor<>()
			.property("state", &JointComponent::state);

		rttr::registration::class_<FixedJointComponent>("FixedJointComponent")
			.constructor<>();

		rttr::registration::class_<RevoluteJointComponent>("RevoluteJointComponent")
			.constructor<>();

		rttr::registration::class_<DistanceJointComponent>("DistanceJointComponent")
			.constructor<>();
	}

	JointComponent::JointComponent(ref<Entity::EcsData> _data, const BaseDescriptor& _base_descriptor)
		: Component(_data), base_descriptor(_base_descriptor)
	{
	}

	JointComponent::~JointComponent()
	{
	}

	void JointComponent::release()
	{
		PX_RELEASE(joint)
	}

	physx::PxJoint* era_engine::physics::JointComponent::get_native_joint() const
	{
		return joint;
	}

	FixedJointComponent::FixedJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
		: JointComponent(_data, _base_descriptor)
	{
		using namespace physx;

		PxFixedJoint* created_joint = create_fixed_joint(PhysicsUtils::get_body_component(component_data)->get_rigid_actor(),
								   PhysicsUtils::get_body_component(base_descriptor.connected_entity)->get_rigid_actor());
		created_joint->setBreakForce(base_descriptor.break_force, base_descriptor.break_force);
		if (base_descriptor.enable_collision)
		{
			created_joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, true);
		}
		created_joint->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		created_joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

		joint = created_joint;
	}

	FixedJointComponent::~FixedJointComponent()
	{
	}

	RevoluteJointComponent::RevoluteJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor, const RevoluteJointDescriptor& _descriptor)
		: JointComponent(_data, _base_descriptor), descriptor(_descriptor)
	{
		using namespace physx;

		PxRevoluteJoint* created_joint = create_revolute_joint(PhysicsUtils::get_body_component(component_data)->get_rigid_actor(),
			PhysicsUtils::get_body_component(base_descriptor.connected_entity)->get_rigid_actor());
		created_joint->setBreakForce(base_descriptor.break_force, base_descriptor.break_force);
		if (base_descriptor.enable_collision)
		{
			created_joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, true);
		}
		created_joint->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		created_joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

		created_joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
		created_joint->setLimit(PxJointAngularLimitPair{descriptor.lower, descriptor.upper});
		if (descriptor.enable_drive)
		{
			created_joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
			created_joint->setDriveVelocity(descriptor.drive_velocity);
			created_joint->setDriveForceLimit(descriptor.drive_force_limit);
			created_joint->setDriveGearRatio(descriptor.drive_gear_ratio);
		}

		joint = created_joint;
	}

	RevoluteJointComponent::~RevoluteJointComponent()
	{
	}

	DistanceJointComponent::DistanceJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor, const DistanceJointDescriptor& _descriptor)
		: JointComponent(_data, _base_descriptor), descriptor(_descriptor)
	{
		using namespace physx;

		PxDistanceJoint* created_joint = create_distance_joint(PhysicsUtils::get_body_component(component_data)->get_rigid_actor(),
			PhysicsUtils::get_body_component(base_descriptor.connected_entity)->get_rigid_actor());
		created_joint->setBreakForce(base_descriptor.break_force, base_descriptor.break_force);
		if (base_descriptor.enable_collision)
		{
			created_joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, true);
		}
		created_joint->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		created_joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

		created_joint->setTolerance(descriptor.tolerance);

		if (descriptor.enable_min_distance)
		{
			created_joint->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, true);
			created_joint->setMinDistance(descriptor.min_distance);
		}

		if (descriptor.enable_max_distance)
		{
			created_joint->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, true);
			created_joint->setMinDistance(descriptor.max_distance);
		}

		if (descriptor.enable_spring)
		{
			created_joint->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, true);
			created_joint->setDamping(descriptor.damping);
			created_joint->setStiffness(descriptor.stiffness);
		}

		joint = created_joint;
	}

	DistanceJointComponent::~DistanceJointComponent()
	{
	}
}