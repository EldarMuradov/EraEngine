// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include <pch.h>
#include <px/core/px_articulation.h>

physics::px_articulation::px_articulation(const vec3& rootPose, bool fixedBase, bool selfCollision) noexcept
{
	const auto physics = physics_holder::physicsRef->getPhysics();

	articulation = physics->createArticulationReducedCoordinate();

	articulation->setArticulationFlag(PxArticulationFlag::eFIX_BASE, fixedBase);
	articulation->setArticulationFlag(PxArticulationFlag::eDISABLE_SELF_COLLISION, !selfCollision);

	PxTransform rootTrs = PxTransform(createPxVec3(rootPose));

	articulation->setRootGlobalPose(rootTrs);

	PxArticulationLink* link = articulation->createLink(nullptr, rootTrs);

	material = physics->createMaterial(0.8f, 0.8f, 0.6f);

	// Test data
	{
		PxSphereGeometry geometry = PxSphereGeometry(1.0f);

		physx::PxRigidActorExt::createExclusiveShape(*link, geometry, *material);
		physx::PxRigidBodyExt::updateMassAndInertia(*link, 1.0f);

		PxArticulationJointReducedCoordinate* joint = link->getInboundJoint();
		joint->setParentPose(rootTrs);
		joint->setChildPose(rootTrs);

		joint->setJointType(PxArticulationJointType::eREVOLUTE);
		joint->setMotion(PxArticulationAxis::eSWING2, PxArticulationMotion::eLIMITED);
		PxArticulationLimit limits;
		limits.low = -PxPiDivFour;
		limits.high = PxPiDivFour;
		joint->setLimitParams(PxArticulationAxis::eSWING2, limits);

		PxArticulationDrive posDrive;
		posDrive.stiffness = 0.5f;
		posDrive.damping = 0.05f;
		posDrive.maxForce = 100.0f;
		posDrive.driveType = PxArticulationDriveType::eFORCE;

		joint->setDriveParams(PxArticulationAxis::eSWING2, posDrive);
		joint->setDriveVelocity(PxArticulationAxis::eSWING2, 0.0f);
		joint->setDriveTarget(PxArticulationAxis::eSWING2, 1.0f);
	}

	physics_holder::physicsRef->getScene()->addArticulation(*articulation);
}
