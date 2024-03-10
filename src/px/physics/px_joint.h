#pragma once
#include "px/core/px_physics_engine.h"

namespace physics
{
	using namespace physx;

	enum class px_joint_type : uint8
	{
		None,
		Revolute,
		Fixed,
		D6,
		Prismatic,
		Distance,
		Spherical
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
		X = 0,		// Motion along the X axis
		Y = 1,		// Motion along the Y axis
		Z = 2,		// Motion along the Z axis
		Twist = 3,	// Motion around the X axis
		Swing1 = 4,	// Motion around the Y axis
		Swing2 = 5,	// Motion around the Z axis
		All = 6
	};

	enum class px_d6_motion
	{
		Locked,	// The DOF is locked, it does not allow relative motion.
		Limited,	// The DOF is limited, it only allows motion within a specific range.
		Free
	};

	enum class px_d6_drive
	{
		X = 0,		// Drive along the X-axis
		Y = 1,		// Drive along the Y-axis
		Z = 2,		// Drive along the Z-axis
		Swing = 3,	// Drive of displacement from the X-axis
		Twist = 4,	// Drive of the displacement around the X-axis
		Slerp = 5,	// Drive of all three angular degrees along a SLERP-path
		All = 6
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
		Actor1,
		Actor2,
		All
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

	NODISCARD PxRevoluteJoint* createRevoluteJoint(PxRigidActor* f, PxRigidActor* s) noexcept;
	NODISCARD PxDistanceJoint* createDistanceJoint(PxRigidActor* f, PxRigidActor* s) noexcept;
	NODISCARD PxSphericalJoint* createSphericalJoint(PxRigidActor* f, PxRigidActor* s) noexcept;
	NODISCARD PxFixedJoint* createFixedJoint(PxRigidActor* f, PxRigidActor* s) noexcept;
	NODISCARD PxPrismaticJoint* createPrismaticJoint(PxRigidActor* f, PxRigidActor* s) noexcept;
	NODISCARD PxD6Joint* createD6Joint(PxRigidActor* f, PxRigidActor* s) noexcept;

	struct px_joint
	{
		px_joint() = default;
		px_joint(PxRigidActor* f, PxRigidActor* s) noexcept : first(f), second(s) {};
		virtual ~px_joint();

		void init(PxRigidActor* f, PxRigidActor* s) noexcept;

		PxRigidActor* first = nullptr;
		PxRigidActor* second = nullptr;

		px_joint_type type = px_joint_type::None;

	protected:
		PxJoint* joint = nullptr;
	};

	struct px_revolute_joint : px_joint
	{
		px_revolute_joint() = default;
		px_revolute_joint(px_revolute_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept;
		~px_revolute_joint();
	};

	struct px_spherical_joint : px_joint
	{
		px_spherical_joint() = default;
		px_spherical_joint(px_spherical_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept;
		~px_spherical_joint();
	};

	struct px_fixed_joint : px_joint
	{
		px_fixed_joint() = default;
		px_fixed_joint(px_fixed_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept;
		~px_fixed_joint();
	};

	struct px_prismatic_joint : px_joint
	{
		px_prismatic_joint() = default;
		px_prismatic_joint(px_prismatic_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept;
		~px_prismatic_joint();
	};

	struct px_d6_joint : px_joint
	{
		px_d6_joint() = default;
		px_d6_joint(px_d6_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept;
		~px_d6_joint();
	};

	struct px_distance_joint : px_joint
	{
		px_distance_joint() = default;
		px_distance_joint(px_distance_joint_desc desc, PxRigidActor* f, PxRigidActor* s) noexcept;
		~px_distance_joint();
	};
}