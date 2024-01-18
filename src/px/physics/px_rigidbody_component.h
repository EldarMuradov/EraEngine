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

	physx::PxRigidActor* getRigidActor() noexcept;

	void setDisableGravity() noexcept;
	void setEnableGravity() noexcept;

	void setMass(uint32_t mass) noexcept;
	uint32_t getMass() const noexcept;

	void setConstraints(uint8 constraints) noexcept;
	uint8 getConstraints() noexcept;

	void setLinearVelocity(vec3 velocity);

	vec3 getLinearVelocity();

	void setAngularVelocity(vec3 velocity);

	vec3 getAngularVelocity();

	vec3 getPhysicsPosition();

	void setPhysicsPositionAndRotation(vec3& pos, quat& rot);

	void setAngularDamping(float damping);

	px_rigidbody_type getType() const noexcept { return type; }

	void onCollisionEnter(px_rigidbody_component* collision) const;

	uint32_t handle {};

private:
	void createPhysics(bool addToScene);
	physx::PxRigidActor* createActor();

private:
	physx::PxMaterial* material = nullptr;

	physx::PxRigidActor* actor = nullptr;

	float restitution = 0.6f;

	uint32_t mass = 1;

	float dynamicFriction = 0.8f;
	float staticFriction = 0.8f;
 
 	physx::PxU32 filterGroup = -1;
	physx::PxU32 filterMask = -1;

	physx::PxRigidDynamicLockFlags rotLockNative;
	physx::PxRigidDynamicLockFlags posLockNative;

	bool useGravity = true;

	px_rigidbody_type type = px_rigidbody_type::None;
}; 