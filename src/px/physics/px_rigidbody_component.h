#pragma once
#include <px/core/px_physics_engine.h>

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

	NODISCARD physx::PxRigidActor* getRigidActor() const noexcept { return actor; }

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

	NODISCARD px_rigidbody_type getType() const noexcept { return type; }

	void onCollisionEnter(px_rigidbody_component* collision) const;
	void onCollisionExit(px_rigidbody_component* collision) const;
	void onCollisionStay(px_rigidbody_component* collision) const;

	uint32_t handle {};

private:
	void createPhysics(bool addToScene);
	NODISCARD physx::PxRigidActor* createActor();

private:
	physx::PxMaterial* material = nullptr;

	physx::PxRigidActor* actor = nullptr;

	float restitution = 0.6f;

	float mass = 1;

	float dynamicFriction = 0.8f;
	float staticFriction = 0.8f;
 
 	physx::PxU32 filterGroup = -1;
	physx::PxU32 filterMask = -1;

	physx::PxRigidDynamicLockFlags rotLockNative;
	physx::PxRigidDynamicLockFlags posLockNative;

	bool useGravity = true;

	px_rigidbody_type type = px_rigidbody_type::None;
}; 