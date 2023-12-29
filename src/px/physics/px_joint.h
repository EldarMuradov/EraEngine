#pragma once
#include "px/core/px_physics_engine.h"

enum class px_joint_type : uint8
{
	px_none,
	px_revolute,
	px_fixed,
	px_d6,
	px_prismatic,
	px_distance,
	px_spherical
};

struct px_angular_limit_pair
{
	float upper = physx::PxPi / 4;
	float lower = -physx::PxPi / 4;

	float contactDistance = 0.01f;

	float stiffness = 10.0f;
	float damping = 0.01f;
};

struct px_linear_limit_pair
{
	float contactDistance = 0.01f;

	float upper = 0.0f;
	float lower = 0.0f;

	float stiffness = 100.0f;
	float damping = 20.0f;
};

struct px_linear_limit
{
	float contactDistance = 0.01f;

	float value = 0.0f;

	float stiffness = 100.0f;
	float damping = 20.0f;
};

struct px_joint_limit_cone
{
	float contactDistance = 0.01f;

	float yAngle = physx::PxPi / 2;
	float zAngle = physx::PxPi / 6;

	float stiffness = 100.0f;
	float damping = 20.0f;
};

struct px_revolute_joint_drive
{
	bool anabledDrive = false;
	float driveVelocity = 10.0f;
	float driveForceLimit = 100.0f;
	float driveGearRatio = 0.0f;
};

struct px_revolute_joint_desc
{
	px_angular_limit_pair angularPair;
	float projectionLinearTolerance = 0.1f;
	px_revolute_joint_drive drive;
};

struct px_spherical_joint_desc
{
	px_joint_limit_cone limitCone;
	float projectionLinearTolerance = 0.1f;
};

struct px_prismatic_joint_desc
{
	px_linear_limit_pair linearPair;
	float projectionLinearTolerance = 0.1f;
	float projectionAngularTolerance = 0.1f;
};

struct px_fixed_joint_desc 
{
	float projectionLinearTolerance = 0.1f;
	float projectionAngularTolerance = 0.1f;
};

struct px_distance_joint_desc
{
	float minDistance = 10.0f;
	float maxDistance = 100.0f;
	float stiffness = 100.0f;
	float damping = 20.0f;
};

enum class px_d6_axis
{
	px_x = 0,		// Motion along the X axis
	px_y = 1,		// Motion along the Y axis
	px_z = 2,		// Motion along the Z axis
	px_twist = 3,	// Motion around the X axis
	px_swing1 = 4,	// Motion around the Y axis
	px_swing2 = 5,	// Motion around the Z axis
	px_all = 6
};

enum class px_d6_motion 
{
	px_locked,	// The DOF is locked, it does not allow relative motion.
	px_limited,	// The DOF is limited, it only allows motion within a specific range.
	px_free
};

enum class px_d6_drive
{
	px_x = 0,		// Drive along the X-axis
	px_y = 1,		// Drive along the Y-axis
	px_z = 2,		// Drive along the Z-axis
	px_swing = 3,	// Drive of displacement from the X-axis
	px_twist = 4,	// Drive of the displacement around the X-axis
	px_slerp = 5,	// Drive of all three angular degrees along a SLERP-path
	px_all = 6
};

struct px_d6_joint_drive 
{
	bool anabledDrive = false;
	float forceLimit = 0.0f;

	float stiffness = 10.0f;
	float damping = 0.01f;

	px_d6_drive flags;

	bool acceleration = false;
};

enum class px_joint_actor_index
{
	px_actor1,
	px_actor2,
	px_all
};

struct px_d6_joint_desc
{
	px_d6_joint_drive drive;
	float projectionLinearTolerance = 0.1f;
	float projectionAngularTolerance = 0.1f;

	px_linear_limit linearLimit;
	px_linear_limit_pair linearLimitPair;
	px_angular_limit_pair angularLimitPair;
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
	px_joint(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept : first(f), second(s) {};
	virtual ~px_joint();

	void init(physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
	
	physx::PxRigidActor* first = nullptr;
	physx::PxRigidActor* second = nullptr;

	px_joint_type type = px_joint_type::px_none;

protected:
	physx::PxJoint* joint = nullptr;
};

struct px_revolute_joint : px_joint
{
	px_revolute_joint() = default;
	px_revolute_joint(px_revolute_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
	~px_revolute_joint();
};

struct px_spherical_joint : px_joint
{
	px_spherical_joint() = default;
	px_spherical_joint(px_spherical_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
	~px_spherical_joint();
};

struct px_fixed_joint : px_joint
{
	px_fixed_joint() = default;
	px_fixed_joint(px_fixed_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
	~px_fixed_joint();
};

struct px_prismatic_joint : px_joint
{
	px_prismatic_joint() = default;
	px_prismatic_joint(px_prismatic_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
	~px_prismatic_joint();
};

struct px_d6_joint : px_joint
{
	px_d6_joint() = default;
	px_d6_joint(px_d6_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
	~px_d6_joint();
};

struct px_distance_joint : px_joint
{
	px_distance_joint() = default;
	px_distance_joint(px_distance_joint_desc desc, physx::PxRigidActor* f, physx::PxRigidActor* s) noexcept;
	~px_distance_joint();
};