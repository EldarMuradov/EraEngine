#include "pch.h"
#include "px/physics/px_joint.h"

px_joint::~px_joint()
{
	PX_RELEASE(joint)
}

void px_joint::init(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	first = f;
	second = s;
}

physx::PxRevoluteJoint* createRevoluteJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	physx::PxRevoluteJoint* joint = PxRevoluteJointCreate(*px_physics_engine::getPhysics(),
		f, f->getGlobalPose(),
		s, s->getGlobalPose());

	return joint;
}

physx::PxDistanceJoint* createDistanceJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	physx::PxDistanceJoint* joint = PxDistanceJointCreate(*px_physics_engine::getPhysics(),
		f, f->getGlobalPose(),
		s, s->getGlobalPose());
	return joint;
}

physx::PxSphericalJoint* createSphericalJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	physx::PxSphericalJoint* joint = PxSphericalJointCreate(*px_physics_engine::getPhysics(),
		f, f->getGlobalPose(),
		s, s->getGlobalPose());
	return joint;
}

physx::PxFixedJoint* createFixedJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	physx::PxFixedJoint* joint = PxFixedJointCreate(*px_physics_engine::getPhysics(),
		f, f->getGlobalPose(),
		s, s->getGlobalPose());
	return joint;
}

physx::PxPrismaticJoint* createPrismaticJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	physx::PxPrismaticJoint* joint = PxPrismaticJointCreate(*px_physics_engine::getPhysics(),
		f, f->getGlobalPose(),
		s, s->getGlobalPose());
	return joint;
}

physx::PxD6Joint* createD6Joint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	physx::PxD6Joint* joint = PxD6JointCreate(*px_physics_engine::getPhysics(),
		f, f->getGlobalPose(),
		s, s->getGlobalPose());
	return joint;
}

px_revolute_joint::px_revolute_joint(px_revolute_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept : px_joint(f, s)
{
	joint = createRevoluteJoint(f, s);
	type = px_joint_type::px_revolute;
	PxJointAngularLimitPair limitPair(desc.angularPair.lower,
		desc.angularPair.upper,
		desc.angularPair.contactDistance);
	limitPair.stiffness = desc.angularPair.stiffness;
	limitPair.damping = desc.angularPair.damping;

	auto jInstance = joint->is<physx::PxRevoluteJoint>();
	if (desc.drive.anabledDrive)
	{
		jInstance->setDriveVelocity(desc.drive.driveVelocity);
		jInstance->setDriveGearRatio(desc.drive.driveGearRatio);
		jInstance->setDriveForceLimit(desc.drive.driveForceLimit);
		jInstance->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
	}
	jInstance->setProjectionLinearTolerance(desc.projectionLinearTolerance);
	jInstance->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
	jInstance->setLimit(limitPair);
	jInstance->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
}

px_revolute_joint::~px_revolute_joint()
{
}

px_spherical_joint::px_spherical_joint(px_spherical_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	joint = createSphericalJoint(f, s);
	type = px_joint_type::px_spherical;

	PxJointLimitCone limitCone(desc.limitCone.yAngle,
		desc.limitCone.zAngle,
		desc.limitCone.contactDistance);

	limitCone.stiffness = desc.limitCone.stiffness;
	limitCone.damping = desc.limitCone.damping;

	auto jInstance = joint->is<physx::PxSphericalJoint>();
	jInstance->setProjectionLinearTolerance(desc.projectionLinearTolerance);
	jInstance->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
	jInstance->setLimitCone(limitCone);
	jInstance->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
}

px_spherical_joint::~px_spherical_joint()
{
}

px_prismatic_joint::px_prismatic_joint(px_prismatic_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	joint = createPrismaticJoint(f, s);
	type = px_joint_type::px_prismatic;
	PxTolerancesScale ts;
	ts.length = 1.0;
	ts.speed = 981;
	PxJointLinearLimitPair limitPair(ts,
		desc.linearPair.lower,
		desc.linearPair.upper,
		desc.linearPair.contactDistance);

	limitPair.stiffness = desc.linearPair.stiffness;
	limitPair.damping = desc.linearPair.damping;

	auto jInstance = joint->is<physx::PxPrismaticJoint>();
	jInstance->setProjectionLinearTolerance(desc.projectionLinearTolerance);
	jInstance->setProjectionAngularTolerance(desc.projectionAngularTolerance);
	jInstance->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
	jInstance->setLimit(limitPair);
	jInstance->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, true);
}

px_prismatic_joint::~px_prismatic_joint()
{
}

px_distance_joint::px_distance_joint(px_distance_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	joint = createDistanceJoint(f, s);
	type = px_joint_type::px_distance;
	auto jInstance = joint->is<physx::PxDistanceJoint>();
	jInstance->setDamping(desc.damping);
	jInstance->setStiffness(desc.stiffness);
	jInstance->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, true);
	jInstance->setMinDistance(desc.minDistance);
	jInstance->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, true);
	jInstance->setMaxDistance(desc.maxDistance);
	jInstance->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, true);
}

px_distance_joint::~px_distance_joint()
{
}

px_fixed_joint::px_fixed_joint(px_fixed_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	joint = createFixedJoint(f, s);
	type = px_joint_type::px_fixed;
	auto jInstance = joint->is<physx::PxFixedJoint>();
	jInstance->setProjectionLinearTolerance(desc.projectionLinearTolerance);
	jInstance->setProjectionAngularTolerance(desc.projectionAngularTolerance);
	jInstance->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
}

px_fixed_joint::~px_fixed_joint()
{
}

px_d6_joint::px_d6_joint(px_d6_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
{
	joint = createD6Joint(f, s);
	type = px_joint_type::px_d6;
	auto jInstance = joint->is<physx::PxD6Joint>();
	if (desc.drive.anabledDrive)
	{
		auto jointDrive = PxD6JointDrive();
		if (desc.drive.acceleration)
			jointDrive.flags = physx::PxD6JointDriveFlag::eACCELERATION;
		jointDrive.damping = desc.drive.damping;
		jointDrive.forceLimit = desc.drive.forceLimit;
		jointDrive.stiffness = desc.drive.stiffness;
		jInstance->setDrive((physx::PxD6Drive::Enum)desc.drive.flags, jointDrive);
		jInstance->setProjectionLinearTolerance(desc.projectionLinearTolerance);
		jInstance->setProjectionAngularTolerance(desc.projectionAngularTolerance);
		jInstance->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
		jInstance->setTwistLimit(PxJointAngularLimitPair(desc.angularLimitPair.lower, desc.angularLimitPair.upper, desc.angularLimitPair.contactDistance));
		jInstance->setLinearLimit(PxJointLinearLimit(desc.linearLimit.value, PxSpring(desc.linearLimit.stiffness, desc.linearLimit.damping)));
	}
}

px_d6_joint::~px_d6_joint()
{
}
