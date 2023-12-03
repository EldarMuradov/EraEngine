#pragma once
#include "px/core/px_physics_engine.h"

enum class px_joint_type
{
	px_revolute,
	px_fixed,
	px_D6,
	px_prismatic,
	px_distance,
	px_spherical
};

struct px_angular_limit_pair
{
	bool available = true;

	float upper = -physx::PxPi / 4;
	float lower = -physx::PxPi / 4;

	float contactDistance = 10.0f;

	float stiffness = 100.0f;
	float damping = 20.0f;
};

struct px_linear_limit_pair
{
	bool available = true;

	float contactDistance = 10.0f;

	float upper = 0.0f;
	float lower = 0.0f;

	float stiffness = 100.0f;
	float damping = 20.0f;
};

struct px_joint_desc
{
	bool limitAvailable = true;
	px_joint_type type = px_joint_type::px_revolute;
	px_angular_limit_pair angularPair;
	px_linear_limit_pair linearPair;

	float maxDistance = 100.0f;
};

physx::PxRevoluteJoint* createRevoluteJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
physx::PxDistanceJoint* createDistanceJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
physx::PxSphericalJoint* createSphericalJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
physx::PxFixedJoint* createFixedJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
physx::PxPrismaticJoint* createPrismaticJoint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
physx::PxD6Joint* createD6Joint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;

struct px_joint
{
	px_joint() = default;
	px_joint(px_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
	physx::PxRigidActor* first = nullptr;
	physx::PxRigidActor* second = nullptr;

	px_joint_type type;

private:
	physx::PxJoint* joint = nullptr;
};