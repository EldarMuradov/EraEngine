// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"
#include "px/physics/px_joint.h"

namespace era_engine::physics
{
	px_joint::~px_joint()
	{
		PX_RELEASE(joint)
	}

	void px_joint::init(PxRigidActor* f, PxRigidActor* s) noexcept
	{
		first = f;
		second = s;
	}

	NODISCARD PxRevoluteJoint* createRevoluteJoint(PxRigidActor* f, PxRigidActor* s) noexcept
	{
		PxRevoluteJoint* joint = PxRevoluteJointCreate(*physics_holder::physicsRef->getPhysics(),
			f, f->getGlobalPose(),
			s, s->getGlobalPose());

		return joint;
	}

	NODISCARD PxDistanceJoint* createDistanceJoint(PxRigidActor* f, PxRigidActor* s) noexcept
	{
		PxDistanceJoint* joint = PxDistanceJointCreate(*physics_holder::physicsRef->getPhysics(),
			f, f->getGlobalPose(),
			s, s->getGlobalPose());
		return joint;
	}

	NODISCARD PxSphericalJoint* createSphericalJoint(PxRigidActor* f, PxRigidActor* s) noexcept
	{
		PxSphericalJoint* joint = PxSphericalJointCreate(*physics_holder::physicsRef->getPhysics(),
			f, f->getGlobalPose(),
			s, s->getGlobalPose());
		return joint;
	}

	NODISCARD PxFixedJoint* createFixedJoint(PxRigidActor* f, PxRigidActor* s) noexcept
	{
		PxFixedJoint* joint = PxFixedJointCreate(*physics_holder::physicsRef->getPhysics(),
			f, f->getGlobalPose(),
			s, s->getGlobalPose());
		return joint;
	}

	NODISCARD PxPrismaticJoint* createPrismaticJoint(PxRigidActor* f, PxRigidActor* s) noexcept
	{
		PxPrismaticJoint* joint = PxPrismaticJointCreate(*physics_holder::physicsRef->getPhysics(),
			f, f->getGlobalPose(),
			s, s->getGlobalPose());
		return joint;
	}

	NODISCARD PxD6Joint* createD6Joint(PxRigidActor* f, PxRigidActor* s) noexcept
	{
		PxD6Joint* joint = PxD6JointCreate(*physics_holder::physicsRef->getPhysics(),
			f, f->getGlobalPose(),
			s, s->getGlobalPose());
		return joint;
	}

	px_revolute_joint::px_revolute_joint(px_revolute_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept : px_joint(f, s)
	{
		joint = createRevoluteJoint(f, s);
		first = f;
		second = s;
		type = px_joint_type::joint_type_revolute;
		PxJointAngularLimitPair limitPair(desc.angularPair.lower,
			desc.angularPair.upper);
		limitPair.stiffness = desc.angularPair.stiffness;
		limitPair.damping = desc.angularPair.damping;

		auto jInstance = joint->is<PxRevoluteJoint>();
		jInstance->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
		if (desc.drive.anabledDrive)
		{
			jInstance->setDriveVelocity(desc.drive.driveVelocity);
			jInstance->setDriveGearRatio(desc.drive.driveGearRatio);
			jInstance->setDriveForceLimit(desc.drive.driveForceLimit);
			jInstance->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
		}
		jInstance->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		jInstance->setLimit(limitPair);
		jInstance->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
	}

	px_revolute_joint::~px_revolute_joint()
	{
	}

	px_spherical_joint::px_spherical_joint(px_spherical_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept
	{
		joint = createSphericalJoint(f, s);
		first = f;
		second = s;
		type = px_joint_type::joint_type_spherical;

		PxJointLimitCone limitCone(desc.limitCone.yAngle,
			desc.limitCone.zAngle);

		limitCone.stiffness = desc.limitCone.stiffness;
		limitCone.damping = desc.limitCone.damping;

		auto jInstance = joint->is<PxSphericalJoint>();
		jInstance->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
		jInstance->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		jInstance->setLimitCone(limitCone);
		jInstance->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
	}

	px_spherical_joint::~px_spherical_joint()
	{
	}

	px_prismatic_joint::px_prismatic_joint(px_prismatic_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept
	{
		joint = createPrismaticJoint(f, s);
		first = f;
		second = s;
		type = px_joint_type::joint_type_prismatic;
		PxTolerancesScale ts;
		ts.length = 1.0;
		ts.speed = 981;

		PxJointLinearLimitPair limitPair(ts,
			desc.linearPair.lower,
			desc.linearPair.upper);

		limitPair.stiffness = desc.linearPair.stiffness;
		limitPair.damping = desc.linearPair.damping;

		auto jInstance = joint->is<PxPrismaticJoint>();
		jInstance->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
		jInstance->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		jInstance->setLimit(limitPair);
		jInstance->setPrismaticJointFlag(PxPrismaticJointFlag::eLIMIT_ENABLED, true);
	}

	px_prismatic_joint::~px_prismatic_joint()
	{
	}

	px_distance_joint::px_distance_joint(px_distance_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept
	{
		joint = createDistanceJoint(f, s);
		first = f;
		second = s;
		type = px_joint_type::joint_type_distance;
		auto jInstance = joint->is<PxDistanceJoint>();
		jInstance->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
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

	px_fixed_joint::px_fixed_joint(px_fixed_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept
	{
		first = f;
		second = s;
		joint = createFixedJoint(f, s);
		type = px_joint_type::joint_type_fixed;
		auto jInstance = joint->is<PxFixedJoint>();
		jInstance->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
		jInstance->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, false);
#if PX_GPU_BROAD_PHASE
		jInstance->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
#endif
		//jInstance->setConstraintFlag(PxConstraintFlag::eALWAYS_UPDATE, true);
		jInstance->setBreakForce(desc.forceThreshold, desc.torqueThreshold);
	}

	px_fixed_joint::~px_fixed_joint()
	{
	}

	px_d6_joint::px_d6_joint(px_d6_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept
	{
		joint = createD6Joint(f, s);
		first = f;
		second = s;
		type = px_joint_type::joint_type_d6;
		auto jInstance = joint->is<PxD6Joint>();
		jInstance->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);
		if (desc.drive.anabledDrive)
		{
			auto jointDrive = PxD6JointDrive();

			if (desc.drive.acceleration)
				jointDrive.flags = PxD6JointDriveFlag::eACCELERATION;
			jointDrive.damping = desc.drive.damping;
			jointDrive.forceLimit = desc.drive.forceLimit;
			jointDrive.stiffness = desc.drive.stiffness;
			jInstance->setDrive((PxD6Drive::Enum)desc.drive.flags, jointDrive);
			jInstance->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
			jInstance->setTwistLimit(PxJointAngularLimitPair(desc.angularLimitPair.lower, desc.angularLimitPair.upper));
			jInstance->setLinearLimit(PxJointLinearLimit(desc.linearLimit.value, PxSpring(desc.linearLimit.stiffness, desc.linearLimit.damping)));
		}
	}

	px_d6_joint::~px_d6_joint()
	{
	}

}