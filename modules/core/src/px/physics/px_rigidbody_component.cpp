// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/core/px_physics_engine.h"
#include "px/physics/px_rigidbody_component.h"
#include "px/physics/px_collider_component.h"
#include "px/core/px_extensions.h"

#include "core/math.h"

#include "scene/scene.h"

#include "application.h"

namespace era_engine::physics
{
	px_body_component::px_body_component(uint32 handle) : px_physics_component_base(handle)
	{
	}

	px_body_component::~px_body_component()
	{
	}

	void px_body_component::setMass(float mass)
	{
	}

	void px_body_component::setPhysicsPositionAndRotation(const vec3& pos, const quat& rot)
	{
		physics_lock_write lock{};

		actor->setGlobalPose(PxTransform(createPxVec3(pos), createPxQuat(rot)));

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			if (dyn->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
				return;
			dyn->setAngularVelocity(PxVec3(0.0f));
			dyn->setLinearVelocity(PxVec3(0.0f));
		}
	}

	uint32 px_body_component::getFilterMask() const
	{
		return filterMask;
	}

	void px_body_component::setFilterMask(uint32 group, uint32 mask)
	{
		auto& physicsRef = physics_holder::physicsRef;

		filterMask = mask;
		filterGroup = group;
		auto& colliders = physicsRef->collidersMap[entityHandle];

		for (auto& collider : colliders)
		{
			setupFiltering(collider->getShape(), filterGroup, filterMask);
		}
	}

	void px_body_component::onCollisionEnter(px_body_component* collision) const
	{
	}

	void px_body_component::onCollisionExit(px_body_component* collision) const
	{
	}

	void px_body_component::onCollisionStay(px_body_component* collision) const
	{
	}

	px_dynamic_body_component::px_dynamic_body_component(uint32 handle, bool addToScene) : px_body_component(handle)
	{
		eentity entity = { entityHandle, &globalApp.getCurrentScene()->registry };

		auto& physicsRef = physics_holder::physicsRef;
		const auto& physics = physicsRef->getPhysics();

		auto& colliders = physicsRef->collidersMap[entityHandle];
		if (colliders.empty())
		{
			entity.addComponent<px_capsule_collider_component>(1.0f, 2.0f);
			colliders.push_back(&entity.getComponent<px_capsule_collider_component>());
		}

		auto transform = entity.getComponentIfExists<transform_component>();

		if (!transform)
		{
			entity.addComponent<transform_component>(vec3(0.0f), quat::identity);
			transform = &entity.getComponent<transform_component>();
		}

		vec3 pos = transform->position;
		PxVec3 pospx = createPxVec3(pos);

		quat q = transform->rotation;
		PxQuat rotpx = createPxQuat(q);

		userData[0] = (uint32_t)entityHandle;

		material = physics->createMaterial(staticFriction, dynamicFriction, restitution);

		actor = createRigidDynamic(PxTransform(pospx, rotpx), (void*)userData);

		for (auto& coll : colliders)
		{
			coll->createGeometry();
			PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, *coll->getGeometry(), *material);
			enableShapeVisualization(shape);
			shape->userData = userData;

			coll->setShape(shape);
		}

		setFilterMask(filterGroup, filterMask);

		physicsRef->addActor(this, actor, addToScene);
	}

	px_dynamic_body_component::~px_dynamic_body_component()
	{
	}

	void px_dynamic_body_component::addForce(const vec3& force, px_force_mode mode)
	{
		if (PxRigidDynamic* dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};

			if (mode == px_force_mode::force_mode_force)
				dyn->addForce(createPxVec3(force), PxForceMode::eFORCE);
			else if (mode == px_force_mode::force_mode_impulse)
				dyn->addForce(createPxVec3(force), PxForceMode::eIMPULSE);
			else if (mode == px_force_mode::force_mode_velocity_change)
				dyn->addForce(createPxVec3(force), PxForceMode::eVELOCITY_CHANGE);
			else if (mode == px_force_mode::force_mode_acceleration)
				dyn->addForce(createPxVec3(force), PxForceMode::eACCELERATION);
			else
				dyn->addForce(createPxVec3(force), PxForceMode::eFORCE);
		}
	}

	void px_dynamic_body_component::addTorque(const vec3& torque, px_force_mode mode)
	{
		if (PxRigidDynamic* dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};

			if (mode == px_force_mode::force_mode_force)
				dyn->addTorque(createPxVec3(torque), PxForceMode::eFORCE);
			else if (mode == px_force_mode::force_mode_impulse)
				dyn->addTorque(createPxVec3(torque), PxForceMode::eIMPULSE);
			else if (mode == px_force_mode::force_mode_velocity_change)
				dyn->addTorque(createPxVec3(torque), PxForceMode::eVELOCITY_CHANGE);
			else if (mode == px_force_mode::force_mode_acceleration)
				dyn->addTorque(createPxVec3(torque), PxForceMode::eACCELERATION);
			else
				dyn->addTorque(createPxVec3(torque), PxForceMode::eFORCE);
		}
	}

	void px_dynamic_body_component::setGravity(bool useGravityFlag)
	{
		actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !useGravityFlag);
		useGravity = useGravityFlag;
	}

	void px_dynamic_body_component::setConstraints(uint8 constraints)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setRigidDynamicLockFlags((PxRigidDynamicLockFlags)constraints);
		}
	}

	NODISCARD uint8 px_dynamic_body_component::getConstraints() const
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_read lock{};
			return dyn->getRigidDynamicLockFlags();
		}
		return 0;
	}

	void px_dynamic_body_component::clearForceAndTorque()
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->clearForce();
			dyn->clearTorque();
		}
	}

	void px_dynamic_body_component::setCCD(bool enable)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
		}
	}

	void px_dynamic_body_component::setMassSpaceInertiaTensor(const vec3& tensor)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setMassSpaceInertiaTensor(createPxVec3(tensor));
		}
	}

	void px_dynamic_body_component::updateMassAndInertia(float density)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			PxRigidBodyExt::updateMassAndInertia(*dyn, density);
		}
	}

	void px_dynamic_body_component::setMaxContactImpulseFlag(bool state)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD_MAX_CONTACT_IMPULSE, state);
		}
	}

	void px_dynamic_body_component::setKinematic(bool kinematic)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, !kinematic);
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, !kinematic);
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD_FRICTION, !kinematic);
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
			isKinematic = kinematic;
		}
	}

	void px_dynamic_body_component::setLinearVelocity(const vec3& velocity)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setLinearVelocity(createPxVec3(velocity));
		}
	}

	NODISCARD vec3 px_dynamic_body_component::getLinearVelocity() const
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_read lock{};
			PxVec3 vel = dyn->getLinearVelocity();
			return createVec3(vel);
		}
		return vec3();
	}

	void px_dynamic_body_component::setAngularVelocity(const vec3& velocity)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setAngularVelocity(createPxVec3(velocity));
		}
	}

	void px_dynamic_body_component::setMaxLinearVelosity(float velocity)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setMaxLinearVelocity(velocity);
		}
	}

	void px_dynamic_body_component::setMaxAngularVelosity(float velocity)
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setMaxAngularVelocity(velocity);
		}
	}

	NODISCARD vec3 px_dynamic_body_component::getAngularVelocity() const
	{
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_read lock{};
			PxVec3 vel = dyn->getAngularVelocity();
			return createVec3(vel);
		}
		return vec3();
	}

	NODISCARD vec3 px_dynamic_body_component::getPhysicsPosition() const
	{
		physics_lock_read lock{};
		PxVec3 pos = actor->getGlobalPose().p;
		return createVec3(pos);
	}

	void px_dynamic_body_component::setAngularDamping(float damping)
	{
		physics_lock_write lock{};
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			dyn->setAngularDamping(damping);
		}
	}

	void px_dynamic_body_component::setLinearDamping(float damping)
	{
		physics_lock_write lock{};
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			dyn->setLinearDamping(damping);
		}
	}

	void px_dynamic_body_component::setThreshold(float stabilization, float sleep)
	{
		physics_lock_write lock{};
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			dyn->setStabilizationThreshold(stabilization);
			dyn->setSleepThreshold(sleep);
		}
	}

	void px_dynamic_body_component::release(bool releaseActor)
	{
		physics_holder::physicsRef->removeActor(this);

		PX_RELEASE(material)

		if (releaseActor)
			PX_RELEASE(actor)
	}

	px_static_body_component::px_static_body_component(uint32 handle, bool addToScene) : px_body_component(handle)
	{
		eentity entity = { entityHandle, &globalApp.getCurrentScene()->registry };

		auto& physicsRef = physics_holder::physicsRef;
		const auto& physics = physicsRef->getPhysics();

		auto& colliders = physicsRef->collidersMap[entityHandle];
		if (colliders.empty())
		{
			entity.addComponent<px_capsule_collider_component>(1.0f, 2.0f);
			colliders.push_back(&entity.getComponent<px_capsule_collider_component>());
		}

		auto transform = entity.getComponentIfExists<transform_component>();

		if (!transform)
		{
			entity.addComponent<transform_component>(vec3(0.0f), quat::identity);
			transform = &entity.getComponent<transform_component>();
		}

		vec3 pos = transform->position;
		PxVec3 pospx = createPxVec3(pos);

		quat q = transform->rotation;
		PxQuat rotpx = createPxQuat(q);

		userData[0] = (uint32_t)entityHandle;

		material = physics->createMaterial(staticFriction, dynamicFriction, restitution);

		actor = createRigidStatic(PxTransform(pospx, rotpx), (void*)userData);

		for (auto& coll : colliders)
		{
			coll->createGeometry();
			PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, *coll->getGeometry(), *material);
			enableShapeVisualization(shape);
			shape->userData = userData;

			coll->setShape(shape);
		}

		setFilterMask(filterGroup, filterMask);

		physicsRef->addActor(this, actor, addToScene);
	}

	px_static_body_component::~px_static_body_component()
	{
	}

	void px_static_body_component::release(bool releaseActor)
	{
		physics_holder::physicsRef->removeActor(this);

		PX_RELEASE(material)

		if (releaseActor)
			PX_RELEASE(actor)
	}

	PxRigidDynamic* createRigidDynamic(const PxTransform& transform, void* userData)
	{
		auto& physicsRef = physics_holder::physicsRef;
		const auto& physics = physicsRef->getPhysics();

		PxRigidDynamic* actor = physics->createRigidDynamic(transform);

		ASSERT(actor != nullptr);

		actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
		actor->userData = userData;

		return actor;
	}

	PxRigidStatic* createRigidStatic(const PxTransform& transform, void* userData)
	{
		auto& physicsRef = physics_holder::physicsRef;
		const auto& physics = physicsRef->getPhysics();

		PxRigidStatic* actor = physics->createRigidStatic(transform);

		ASSERT(actor != nullptr);

		actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
		actor->userData = userData;

		return actor;
	}

}