#include "pch.h"
#include "px/core/px_physics_engine.h"
#include <scene/scene.h>
#include <px/physics/px_collider_component.h>
#include <core/log.h>
#include "core/math.h"
#include "application.h"

px_rigidbody_component::px_rigidbody_component(uint32_t entt, px_rigidbody_type rbtype, bool addToScene) noexcept : type(rbtype)
{
	handle = entt;
	if(rbtype != px_rigidbody_type::None)
		createPhysics(addToScene);
}

px_rigidbody_component::~px_rigidbody_component()
{
}

void px_rigidbody_component::addForce(vec3 force, px_force_mode mode) noexcept
{
	if (mode == px_force_mode::Force)
		actor->is<PxRigidDynamic>()->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eFORCE);
	else if (mode == px_force_mode::Impulse)
		actor->is<PxRigidDynamic>()->addForce(PxVec3(force.x, force.y, force.z), PxForceMode::eIMPULSE);
}

void px_rigidbody_component::setDisableGravity() noexcept
{
	actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
	useGravity = false;
}

void px_rigidbody_component::setEnableGravity() noexcept
{
	actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, false);
	useGravity = true;
}

void px_rigidbody_component::setMass(float mass) noexcept
{
	this->mass = mass;
	actor->is<PxRigidDynamic>()->setMass(mass);
}

void px_rigidbody_component::setConstraints(uint8 constraints) noexcept
{
	actor->is<PxRigidDynamic>()->setRigidDynamicLockFlags((physx::PxRigidDynamicLockFlags)constraints);
}

NODISCARD uint8 px_rigidbody_component::getConstraints() const noexcept
{
	return (uint8)actor->is<PxRigidDynamic>()->getRigidDynamicLockFlags();
}

void px_rigidbody_component::setLinearVelocity(vec3 velocity)
{
	if(actor->is<PxRigidDynamic>())
		actor->is<PxRigidDynamic>()->setLinearVelocity(PxVec3(velocity.x, velocity.y, velocity.z));
}

NODISCARD vec3 px_rigidbody_component::getLinearVelocity() const noexcept
{
	if (actor->is<PxRigidDynamic>())
	{
		PxVec3 vel = actor->is<PxRigidDynamic>()->getLinearVelocity();
		return vec3(vel.x, vel.y, vel.z);
	}
	return vec3();
}

void px_rigidbody_component::setAngularVelocity(vec3 velocity)
{
	if (actor->is<PxRigidDynamic>())
		actor->is<PxRigidDynamic>()->setAngularVelocity(PxVec3(velocity.x, velocity.y, velocity.z));
}

NODISCARD vec3 px_rigidbody_component::getAngularVelocity() const noexcept
{
	if (actor->is<PxRigidDynamic>())
	{
		PxVec3 vel = actor->is<PxRigidDynamic>()->getAngularVelocity();
		return vec3(vel.x, vel.y, vel.z);
	}
	return vec3();
}

NODISCARD vec3 px_rigidbody_component::getPhysicsPosition() const noexcept
{
	PxVec3 pos = actor->getGlobalPose().p;
	return vec3(pos.x, pos.y, pos.z);
}

void px_rigidbody_component::setPhysicsPositionAndRotation(vec3& pos, quat& rot)
{
	setAngularVelocity(vec3(0.0f));
	setLinearVelocity(vec3(0.0f));
	PxQuat nq = physx::createPxQuat(rot);

	actor->setGlobalPose(PxTransform(physx::createPxVec3(pos), nq.getConjugate()));
}

void px_rigidbody_component::setAngularDamping(float damping)
{
	if(damping > 0.0f)
		actor->is<PxRigidDynamic>()->setAngularDamping(damping);
}

void px_rigidbody_component::onCollisionEnter(px_rigidbody_component* collision) const
{

}

void px_rigidbody_component::createPhysics(bool addToScene)
{
	actor = createActor();
	uint32_t* h = new uint32_t[1];
	h[0] = (uint32_t)handle;
	actor->userData = h;

	px_physics_engine::get()->addActor(this, actor, addToScene);

#if PX_ENABLE_PVD
	actor->setActorFlags(physx::PxActorFlag::eVISUALIZATION);
#endif
}

NODISCARD physx::PxRigidActor* px_rigidbody_component::createActor()
{
	eentity entity = { handle, &px_physics_engine::get()->app->getCurrentScene()->registry };
	px_collider_component_base* coll = (px_collider_component_base*)entity.getComponentIfExists<px_sphere_collider_component>();
	if(!coll)
		coll = (px_collider_component_base*)entity.getComponentIfExists<px_box_collider_component>();
	if(!coll)
		coll = (px_collider_component_base*)entity.getComponentIfExists<px_capsule_collider_component>();
	if(!coll)
		coll = (px_collider_component_base*)entity.getComponentIfExists<px_triangle_mesh_collider_component>();
	if (!coll)
		coll = (px_collider_component_base*)entity.getComponentIfExists<px_bounding_box_collider_component>();
	if (!coll)
	{
		entity.addComponent<px_capsule_collider_component>(1.0f, 2.0f);
		coll = (px_collider_component_base*)&entity.getComponent<px_capsule_collider_component>();
	}

	auto tranaform = entity.getComponentIfExists<transform_component>();

	if (!tranaform)
	{
		entity.addComponent<transform_component>(vec3(0.0f), quat::identity);
		tranaform = &entity.getComponent<transform_component>();
	}

	vec3 pos = tranaform->position;
	PxVec3 pospx = physx::createPxVec3(pos);

	quat q = tranaform->rotation;
	PxQuat rotpx = physx::createPxQuat(q);
	rotpx = rotpx.getConjugate();

	material = px_physics_engine::getPhysics()->createMaterial(staticFriction, dynamicFriction, restitution);

	if (type == px_rigidbody_type::Static)
	{
		PxRigidStatic* actor = px_physics_engine::getPhysics()->createRigidStatic(PxTransform(pospx, rotpx));

		coll->createShape();
		uint32_t* h= new uint32_t[1];
		h[0] = (uint32_t)handle;
		coll->getShape()->userData = h;
		actor->attachShape(*coll->getShape());

		return actor;
	}
	else
	{
		PxRigidDynamic* actor = px_physics_engine::getPhysics()->createRigidDynamic(PxTransform(pospx, rotpx));
		actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_POSE_INTEGRATION_PREVIEW, true);
		actor->setRigidBodyFlag(PxRigidBodyFlag::eRETAIN_ACCELERATIONS, true);

#if PX_GPU_BROAD_PHASE
		actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, true);
#else
		actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
		actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD_FRICTION, true);
#endif

		coll->createShape();
		uint32_t* h = new uint32_t[1];
		h[0] = (uint32_t)handle;
		coll->getShape()->userData = h;
		actor->attachShape(*coll->getShape());

		return actor;
	}
}
