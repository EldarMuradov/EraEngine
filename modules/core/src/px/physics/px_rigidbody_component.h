// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "px/core/px_physics_engine.h"

namespace era_engine::physics
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

	struct px_body_component : px_physics_component_base
	{
		px_body_component() {};
		px_body_component(uint32 handle);
		virtual ~px_body_component();

		NODISCARD PxRigidActor* getRigidActor() const { return actor; }

		void setMass(float mass);
		NODISCARD float getMass() const { return mass; }

		void setPhysicsPositionAndRotation(const vec3& pos, const quat& rot);

		uint32 getFilterMask() const;
		void setFilterMask(uint32 group, uint32 mask);

		void onCollisionEnter(px_body_component* collision) const;
		void onCollisionExit(px_body_component* collision) const;
		void onCollisionStay(px_body_component* collision) const;

	protected:
		PxRigidActor* actor = nullptr;

		PxMaterial* material = nullptr;

		uint32_t* userData = new uint32_t[1];

		PxU32 filterGroup = -1;
		PxU32 filterMask = -1;

		float mass = 1;

		float restitution = 0.6f;
		float dynamicFriction = 0.8f;
		float staticFriction = 0.8f;
	};

	struct px_dynamic_body_component : px_body_component
	{
		px_dynamic_body_component() {};
		px_dynamic_body_component(uint32 handle, bool addToScene = true);
		virtual ~px_dynamic_body_component();

		void addForce(const vec3& force, px_force_mode mode = px_force_mode::force_mode_impulse);
		void addTorque(const vec3& torque, px_force_mode mode = px_force_mode::force_mode_impulse);

		NODISCARD PxRigidDynamic* getRigidDynamic() const { return actor->is<PxRigidDynamic>(); }

		void setGravity(bool useGravityFlag);

		void setConstraints(uint8 constraints);
		NODISCARD uint8 getConstraints() const;

		void clearForceAndTorque();

		void setCCD(bool enable);

		void setMassSpaceInertiaTensor(const vec3& tensor);

		void updateMassAndInertia(float density);

		void setMaxContactImpulseFlag(bool state);

		void setKinematic(bool kinematic);
		bool isKinematicBody() const { return isKinematic; }

		void setLinearVelocity(const vec3& velocity);

		NODISCARD vec3 getLinearVelocity() const;

		void setAngularVelocity(const vec3& velocity);

		void setMaxLinearVelosity(float velocity);
		void setMaxAngularVelosity(float velocity);

		NODISCARD vec3 getAngularVelocity() const;

		NODISCARD vec3 getPhysicsPosition() const;

		void setAngularDamping(float damping);
		void setLinearDamping(float damping);

		void setThreshold(float stabilization = 0.01f, float sleep = 0.01f);

		virtual void release(bool releaseActor = false) override;

	protected:
		PxRigidDynamicLockFlags rotLockNative;
		PxRigidDynamicLockFlags posLockNative;

		bool enableCCD = false;

		bool useGravity = true;
		bool isKinematic = false;
	};

	struct px_static_body_component : px_body_component
	{
		px_static_body_component() {};
		px_static_body_component(uint32 handle, bool addToScene = true);
		virtual ~px_static_body_component();

		NODISCARD PxRigidStatic* getRigidStatic() const { return actor->is<PxRigidStatic>(); }

		virtual void release(bool releaseActor = false) override;
	};

	PxRigidDynamic* createRigidDynamic(const PxTransform& transform, void* userData);
	PxRigidStatic* createRigidStatic(const PxTransform& transform, void* userData);
}

#include "core/reflect.h"

namespace era_engine
{
	// TODO body reflect
}