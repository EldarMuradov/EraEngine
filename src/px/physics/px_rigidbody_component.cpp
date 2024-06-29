// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"
#include "px/core/px_physics_engine.h"
#include <scene/scene.h>
#include <px/physics/px_collider_component.h>
#include <core/log.h>
#include "core/math.h"
#include "application.h"
#include "px_rigidbody_component.h"
#include <px/core/px_extensions.h>

namespace physics
{
	px_rigidbody_component::px_rigidbody_component(uint32_t handle, px_rigidbody_type rbtype, bool addToScene) : px_physics_component_base(handle), type(rbtype)
	{
		if (rbtype != px_rigidbody_type::rigidbody_type_none)
			createPhysics(addToScene);
	}

	px_rigidbody_component::~px_rigidbody_component()
	{
	}

	void px_rigidbody_component::addForce(vec3 force, px_force_mode mode) noexcept
	{
		if (!actor)
			return;
		physics_lock_write lock{};
		if (mode == px_force_mode::force_mode_force)
			actor->is<PxRigidDynamic>()->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eFORCE);
		else if (mode == px_force_mode::force_mode_impulse)
			actor->is<PxRigidDynamic>()->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eIMPULSE);
		else if (mode == px_force_mode::force_mode_velocity_change)
			actor->is<PxRigidDynamic>()->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eVELOCITY_CHANGE);
		else if (mode == px_force_mode::force_mode_acceleration)
			actor->is<PxRigidDynamic>()->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eACCELERATION);
		else
			actor->is<PxRigidDynamic>()->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eFORCE);
	}

	void px_rigidbody_component::addTorque(vec3 torque, px_force_mode mode) noexcept
	{
		if (!actor)
			return;
		physics_lock_write lock{};
		if (mode == px_force_mode::force_mode_force)
			actor->is<PxRigidDynamic>()->addTorque(PxVec3(torque.x, torque.y, torque.z), PxForceMode::eFORCE);
		else if (mode == px_force_mode::force_mode_impulse)
			actor->is<PxRigidDynamic>()->addTorque(PxVec3(torque.x, torque.y, torque.z), PxForceMode::eIMPULSE);
		else if (mode == px_force_mode::force_mode_velocity_change)
			actor->is<PxRigidDynamic>()->addTorque(PxVec3(torque.x, torque.y, torque.z), PxForceMode::eVELOCITY_CHANGE);
		else if (mode == px_force_mode::force_mode_acceleration)
			actor->is<PxRigidDynamic>()->addTorque(PxVec3(torque.x, torque.y, torque.z), PxForceMode::eACCELERATION);
		else
			actor->is<PxRigidDynamic>()->addTorque(PxVec3(torque.x, torque.y, torque.z), PxForceMode::eFORCE);
	}

	void px_rigidbody_component::setGravity(bool useGravityFlag) noexcept
	{
		if (!actor)
			return;
		actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, useGravityFlag);
		useGravity = useGravityFlag;
	}

	void px_rigidbody_component::setMass(float mass) noexcept
	{
		if (!actor)
			return;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setMass(mass);
			this->mass = mass;
		}
	}

	void px_rigidbody_component::setConstraints(uint8 constraints) noexcept
	{
		if (!actor)
			return;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setRigidDynamicLockFlags((physx::PxRigidDynamicLockFlags)constraints);
		}
	}

	NODISCARD uint8 px_rigidbody_component::getConstraints() const noexcept
	{
		if (!actor)
			return 0;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_read lock{};
			return dyn->getRigidDynamicLockFlags();
		}
		return 0;
	}

	void px_rigidbody_component::clearForceAndTorque() noexcept
	{
		if (!actor)
			return;
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->clearForce();
			dyn->clearTorque();
		}
	}

	void px_rigidbody_component::setMassSpaceInertiaTensor(const vec3& tensor) noexcept
	{
		if (!actor)
			return;
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setMassSpaceInertiaTensor(physx::createPxVec3(tensor));
		}
	}

	void px_rigidbody_component::updateMassAndInertia(float density) noexcept
	{
		if (!actor)
			return;
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			PxRigidBodyExt::updateMassAndInertia(*dyn, density);
		}
	}

	void px_rigidbody_component::setMaxContactImpulseFlag(bool state) noexcept
	{
		if (!actor)
			return;
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD_MAX_CONTACT_IMPULSE, state);
		}
	}

	void px_rigidbody_component::setKinematic(bool kinematic)
	{
		if (!actor)
			return;
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

	void px_rigidbody_component::setLinearVelocity(vec3 velocity)
	{
		if (!actor)
			return;
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setLinearVelocity(PxVec3(velocity.x, velocity.y, velocity.z));
		}
	}

	NODISCARD vec3 px_rigidbody_component::getLinearVelocity() const noexcept
	{
		if (!actor)
			return vec3();
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_read lock{};
			PxVec3 vel = dyn->getLinearVelocity();
			return vec3(vel.x, vel.y, vel.z);
		}
		return vec3();
	}

	void px_rigidbody_component::setAngularVelocity(vec3 velocity)
	{
		if (!actor)
			return;
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setAngularVelocity(PxVec3(velocity.x, velocity.y, velocity.z));
		}
	}

	void px_rigidbody_component::setMaxLinearVelosity(float velocity) noexcept
	{
		if (!actor)
			return;
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setMaxLinearVelocity(velocity);
		}
	}

	void px_rigidbody_component::setMaxAngularVelosity(float velocity) noexcept
	{
		if (!actor)
			return;
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_write lock{};
			dyn->setMaxAngularVelocity(velocity);
		}
	}

	NODISCARD vec3 px_rigidbody_component::getAngularVelocity() const noexcept
	{
		if (!actor)
			return vec3();

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			physics_lock_read lock{};
			PxVec3 vel = dyn->getAngularVelocity();
			return vec3(vel.x, vel.y, vel.z);
		}
		return vec3();
	}

	NODISCARD vec3 px_rigidbody_component::getPhysicsPosition() const noexcept
	{
		if (!actor)
			return vec3();
		physics_lock_read lock{};
		PxVec3 pos = actor->getGlobalPose().p;
		return vec3(pos.x, pos.y, pos.z);
	}

	void px_rigidbody_component::setPhysicsPositionAndRotation(vec3& pos, quat& rot)
	{
		if (!actor)
			return;
		physics_lock_write lock{};

		setAngularVelocity(vec3(0.0f));
		setLinearVelocity(vec3(0.0f));

		actor->setGlobalPose(PxTransform(physx::createPxVec3(pos), physx::createPxQuat(rot)));
	}

	void px_rigidbody_component::setAngularDamping(float damping)
	{
		if (!actor)
			return;
		physics_lock_write lock{};
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			dyn->setAngularDamping(damping);
		}
	}

	void px_rigidbody_component::setLinearDamping(float damping)
	{
		if (!actor)
			return;
		physics_lock_write lock{};
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			dyn->setLinearDamping(damping);
		}
	}

	void px_rigidbody_component::setThreshold(float stabilization, float sleep)
	{
		if (!actor)
			return;
		physics_lock_write lock{};
		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			dyn->setStabilizationThreshold(stabilization);
			dyn->setSleepThreshold(sleep);
		}
	}

	void px_rigidbody_component::release(bool releaseActor) noexcept
	{
		if (!actor)
			return;

		physics_holder::physicsRef->removeActor(this);

		PX_RELEASE(material)
		if(releaseActor)
			PX_RELEASE(actor)
	}

	void px_rigidbody_component::onCollisionEnter(px_rigidbody_component* collision) const
	{
		std::cout << "enter\n";
	}

	void px_rigidbody_component::onCollisionExit(px_rigidbody_component* collision) const
	{
		std::cout << "exit\n";
	}

	void px_rigidbody_component::onCollisionStay(px_rigidbody_component* collision) const
	{
	}

	void px_rigidbody_component::createPhysics(bool addToScene)
	{
		physics_lock_write lock{};

		actor = createActor();
		userData[0] = (uint32_t)entityHandle;
		actor->userData = userData;

		physics_holder::physicsRef->addActor(this, actor, addToScene);

#if PX_ENABLE_PVD && !PX_GPU_BROAD_PHASE
		actor->setActorFlags(physx::PxActorFlag::eVISUALIZATION);
#endif
	}

	NODISCARD PxRigidActor* px_rigidbody_component::createActor()
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
		PxVec3 pospx = physx::createPxVec3(pos);

		quat q = transform->rotation;
		PxQuat rotpx = physx::createPxQuat(q);

		material = physics->createMaterial(staticFriction, dynamicFriction, restitution);

		PxRigidActor* actor = nullptr;

		if (type == px_rigidbody_type::rigidbody_type_static)
		{
			actor = physics->createRigidStatic(PxTransform(pospx, rotpx));

#if PX_ENABLE_PVD && !PX_GPU_BROAD_PHASE
			actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
#endif
		}
		else if(type == px_rigidbody_type::rigidbody_type_dynamic)
		{
			actor = physics->createRigidDynamic(PxTransform(pospx, rotpx));
			//actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);

#if PX_ENABLE_PVD && !PX_GPU_BROAD_PHASE
			actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
#endif
		}
		else
		{
			actor = physics->createRigidDynamic(PxTransform(pospx, rotpx));

			setKinematic(true);

#if PX_ENABLE_PVD && !PX_GPU_BROAD_PHASE
			actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
#endif
		}

		for (auto& coll : colliders)
		{
			coll->createGeometry();
			PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, *coll->getGeometry(), *material);
			shape->userData = userData;
			enableShapeVisualization(shape);
		}

		return actor;
	}
}