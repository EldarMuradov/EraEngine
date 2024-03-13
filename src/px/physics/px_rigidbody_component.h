#pragma once
#include <px/core/px_physics_engine.h>

namespace physics
{
	using namespace physx;

	enum class px_rigidbody_type : uint8
	{
		None,
		Static,
		Dynamic,
		Kinematic
	};

	enum class px_force_mode : uint8
	{
		None,
		Force,
		Impulse
	};

	struct px_rigidbody_component
	{
		px_rigidbody_component() {};
		px_rigidbody_component(uint32_t entt, px_rigidbody_type rbtype, bool addToScene = true) noexcept;
		virtual ~px_rigidbody_component();

		void addForce(vec3 force, px_force_mode mode = px_force_mode::Impulse) noexcept;

		NODISCARD PxRigidActor* getRigidActor() const noexcept { return actor; }

		void setDisableGravity() noexcept;
		void setEnableGravity() noexcept;

		void setMass(float mass) noexcept;
		NODISCARD float getMass() const noexcept { return mass; }

		void setConstraints(uint8 constraints) noexcept;
		NODISCARD uint8 getConstraints() const noexcept;

		void setLinearVelocity(vec3 velocity);

		NODISCARD vec3 getLinearVelocity() const noexcept;

		void setAngularVelocity(vec3 velocity);

		NODISCARD vec3 getAngularVelocity() const noexcept;

		NODISCARD vec3 getPhysicsPosition() const noexcept;

		void setPhysicsPositionAndRotation(vec3& pos, quat& rot);

		void setAngularDamping(float damping);

		virtual void release();

		void onCollisionEnter(px_rigidbody_component* collision) const;
		void onCollisionExit(px_rigidbody_component* collision) const;
		void onCollisionStay(px_rigidbody_component* collision) const;

		uint32_t handle{};
		px_rigidbody_type type = px_rigidbody_type::None;

	private:
		void createPhysics(bool addToScene);
		NODISCARD PxRigidActor* createActor();

	protected:
		PxMaterial* material = nullptr;

		PxRigidActor* actor = nullptr;

		float restitution = 0.6f;

		float mass = 1;

		float dynamicFriction = 0.8f;
		float staticFriction = 0.8f;

		PxU32 filterGroup = -1;
		PxU32 filterMask = -1;

		PxRigidDynamicLockFlags rotLockNative;
		PxRigidDynamicLockFlags posLockNative;

		bool useGravity = true;
	};
}

#include "core/reflect.h"

REFLECT_STRUCT(physics::px_rigidbody_component,
	(handle, "Handle"),
	(type, "Type")
);