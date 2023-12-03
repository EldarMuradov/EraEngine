#include "pch.h"
#include "px/physics/px_joint.h"

px_joint::px_joint(px_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept
	: first(f), second(s)
{
	type = desc.type;
	if (desc.type == px_joint_type::px_revolute)
	{
		joint = createRevoluteJoint(f, s);
		if (desc.limitAvailable)
		{
			if (desc.angularPair.available)
			{
				PxJointAngularLimitPair limitPair(desc.angularPair.lower,
					desc.angularPair.upper,
					desc.angularPair.contactDistance);
				limitPair.stiffness = desc.angularPair.stiffness;
				limitPair.damping = desc.angularPair.damping;

				joint->is<physx::PxRevoluteJoint>()->setLimit(limitPair);
			}

			joint->is<physx::PxRevoluteJoint>()->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
		}
	}
	else if (desc.type == px_joint_type::px_distance)
	{
		joint = createDistanceJoint(f, s);
	}
	else if (desc.type == px_joint_type::px_D6)
	{
		joint = createD6Joint(f, s);
	}
	else if (desc.type == px_joint_type::px_fixed)
	{
		joint = createFixedJoint(f, s);
	}
	else if (desc.type == px_joint_type::px_spherical)
	{
		joint = createSphericalJoint(f, s);
	}
	else
	{
		joint = createPrismaticJoint(f, s);
	}
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
