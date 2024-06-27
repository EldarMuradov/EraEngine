// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <px/core/px_physics_engine.h>

namespace physics
{
	using namespace physx;

	const static inline uint8 PX_FREEZE_ALL = (uint8)(
		PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y |
		PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z | PxRigidDynamicLockFlag::eLOCK_LINEAR_X |
		PxRigidDynamicLockFlag::eLOCK_LINEAR_Y | PxRigidDynamicLockFlag::eLOCK_LINEAR_Z
		);

	enum class px_rigidbody_type : uint8
	{
		rigidbody_type_none,
		rigidbody_type_static,
		rigidbody_type_dynamic,
		rigidbody_type_kinematic
	};

	enum class px_force_mode : uint8
	{
		force_mode_none,
		force_mode_force,
		force_mode_impulse,
		force_mode_velocity_change,
		force_mode_acceleration
	};

	struct px_rigidbody_component : px_physics_component_base
	{
		px_rigidbody_component() {};
		px_rigidbody_component(uint32 handle, px_rigidbody_type rbtype = px_rigidbody_type::rigidbody_type_none, bool addToScene = true);
		virtual ~px_rigidbody_component();

		void addForce(vec3 force, px_force_mode mode = px_force_mode::force_mode_impulse) noexcept;
		void addTorque(vec3 torque, px_force_mode mode = px_force_mode::force_mode_impulse) noexcept;

		NODISCARD PxRigidActor* getRigidActor() const noexcept { return actor; }

		NODISCARD PxRigidDynamic* getRigidDynamic() const noexcept { return actor->is<PxRigidDynamic>(); }

		NODISCARD PxRigidStatic* getRigidStatic() const noexcept { return actor->is<PxRigidStatic>(); }

		void setGravity(bool useGravityFlag) noexcept;

		void setMass(float mass) noexcept;
		NODISCARD float getMass() const noexcept { return mass; }

		void setConstraints(uint8 constraints) noexcept;
		NODISCARD uint8 getConstraints() const noexcept;

		void clearForceAndTorque() noexcept;

		void setMassSpaceInertiaTensor(const vec3& tensor) noexcept;

		void updateMassAndInertia(float density) noexcept;

		void setMaxContactImpulseFlag(bool state) noexcept;

		void setKinematic(bool kinematic);
		bool isKinematicBody() const noexcept { return isKinematic; }

		void setLinearVelocity(vec3 velocity);

		NODISCARD vec3 getLinearVelocity() const noexcept;

		void setAngularVelocity(vec3 velocity);

		void setMaxLinearVelosity(float velocity) noexcept;
		void setMaxAngularVelosity(float velocity) noexcept;

		NODISCARD vec3 getAngularVelocity() const noexcept;

		NODISCARD vec3 getPhysicsPosition() const noexcept;

		void setPhysicsPositionAndRotation(vec3& pos, quat& rot);

		void setAngularDamping(float damping);
		void setLinearDamping(float damping);

		void setThreshold(float stabilization = 0.01f, float sleep = 0.01f);

		virtual void release(bool releaseActor = false) noexcept override;

		void onCollisionEnter(px_rigidbody_component* collision) const;
		void onCollisionExit(px_rigidbody_component* collision) const;
		void onCollisionStay(px_rigidbody_component* collision) const;

		px_rigidbody_type type = px_rigidbody_type::rigidbody_type_none;

	private:
		void createPhysics(bool addToScene);
		NODISCARD PxRigidActor* createActor();

	protected:
		PxMaterial* material = nullptr;

		PxRigidActor* actor = nullptr;

		uint32_t* userData = new uint32_t[1];

		float restitution = 0.6f;

		float mass = 1;

		float dynamicFriction = 0.8f;
		float staticFriction = 0.8f;

		PxU32 filterGroup = -1;
		PxU32 filterMask = -1;

		PxRigidDynamicLockFlags rotLockNative;
		PxRigidDynamicLockFlags posLockNative;

		bool useGravity = true;
		bool isKinematic = false;
	};
}

#include "core/reflect.h"

REFLECT_STRUCT(physics::px_rigidbody_component,
	(type, "Type")
);