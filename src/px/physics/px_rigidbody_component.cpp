#include "pch.h"
#include "px/core/px_physics_engine.h"
#include <scene/scene.h>
#include <px/physics/px_collider_component.h>
#include <core/log.h>
#include "core/math.h"

px_rigidbody_component::px_rigidbody_component(eentity* entt, px_rigidbody_type rbtype, bool addToScene) noexcept : entity(entt), type(rbtype)
{
	if(rbtype != px_rigidbody_type::None)
		createPhysics(addToScene);
	transform = entity->getComponentIfExists<transform_component>();
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

physx::PxRigidActor* px_rigidbody_component::getRigidActor() noexcept
{
	return actor;
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

void px_rigidbody_component::setMass(unsigned int mass)
{
	this->mass = mass;
	actor->is<PxRigidDynamic>()->setMass(mass);
}

unsigned int px_rigidbody_component::getMass() const noexcept
{
	return mass;
}

void px_rigidbody_component::setConstraints(uint8 constraints) noexcept
{
	actor->is<PxRigidDynamic>()->setRigidDynamicLockFlags((physx::PxRigidDynamicLockFlags)constraints);
}

uint8 px_rigidbody_component::getConstraints() noexcept
{
	return (uint8)actor->is<PxRigidDynamic>()->getRigidDynamicLockFlags();
}

void px_rigidbody_component::setLinearVelocity(vec3& velocity)
{
	if(actor->is<PxRigidDynamic>())
		actor->is<PxRigidDynamic>()->setLinearVelocity(PxVec3(velocity.x, velocity.y, velocity.z));
}

vec3 px_rigidbody_component::getLinearVelocity()
{
	if (actor->is<PxRigidDynamic>())
	{
		PxVec3 vel = actor->is<PxRigidDynamic>()->getLinearVelocity();
		return vec3(vel.x, vel.y, vel.z);
	}
	return vec3();
}

void px_rigidbody_component::setAngularVelocity(vec3& velocity)
{
	if (actor->is<PxRigidDynamic>())
		actor->is<PxRigidDynamic>()->setAngularVelocity(PxVec3(velocity.x, velocity.y, velocity.z));
}

vec3 px_rigidbody_component::getAngularVelocity()
{
	if (actor->is<PxRigidDynamic>())
	{
		PxVec3 vel = actor->is<PxRigidDynamic>()->getAngularVelocity();
		return vec3(vel.x, vel.y, vel.z);
	}
	return vec3();
}

vec3 px_rigidbody_component::getPhysicsPosition()
{
	PxVec3 pos = actor->getGlobalPose().p;
	return vec3(pos.x, pos.y, pos.z);
}

void px_rigidbody_component::setPhysicsPositionAndRotation(vec3& pos, quat& rot)
{
	PxQuat nq = PxQuat(rot.x, rot.y, rot.z, rot.w);

	actor->setGlobalPose(PxTransform(PxVec3(pos.x, pos.y, pos.z), nq.getConjugate()));
}

void px_rigidbody_component::setAngularDamping(float damping)
{
	if(damping > 0.0f)
		actor->is<PxRigidDynamic>()->setAngularDamping(damping);
}

void px_rigidbody_component::onCollisionEnter(px_rigidbody_component* collision)
{
	std::cout << "collide" << "\n";
}

void px_rigidbody_component::createPhysics(bool addToScene)
{
	actor = createActor();
	uint32_t* handle = new uint32_t[1];
	handle[0] = (uint32_t)entity->handle;
	actor->userData = handle;

	px_physics_engine::get()->addActor(this, actor, addToScene);

	actor->setActorFlags(physx::PxActorFlag::eVISUALIZATION);
}

physx::PxRigidActor* px_rigidbody_component::createActor()
{
	px_collider_component_base* coll = (px_collider_component_base*)entity->getComponentIfExists<px_sphere_collider_component>();
	if(!coll)
		coll = (px_collider_component_base*)entity->getComponentIfExists<px_box_collider_component>();
	if(!coll)
		coll = (px_collider_component_base*)entity->getComponentIfExists<px_capsule_collider_component>();
	if(!coll)
		coll = (px_collider_component_base*)entity->getComponentIfExists<px_triangle_mesh_collider_component>();
	if (!coll)
		coll = (px_collider_component_base*)entity->getComponentIfExists<px_bounding_box_collider_component>();
	if (!coll)
		return nullptr;

	auto tranaform = entity->getComponentIfExists<transform_component>();

	if (!tranaform)
		return nullptr;

	vec3 pos = tranaform->position;
	PxVec3 pospx = PxVec3(pos.x, pos.y, pos.z);

	quat q = tranaform->rotation;
	PxQuat rotpx = PxQuat(q.x, q.y, q.z, q.w);
	rotpx = rotpx.getConjugate();

	material = px_physics_engine::getPhysics()->createMaterial(staticFriction, dynamicFriction, restitution);

	if (type == px_rigidbody_type::Static)
	{
		PxRigidStatic* actor = px_physics_engine::getPhysics()->createRigidStatic(PxTransform(pospx, rotpx));

		coll->createShape();
		actor->attachShape(*coll->getShape());

		return actor;
	}
	else
	{
		PxRigidDynamic* actor = px_physics_engine::getPhysics()->createRigidDynamic(PxTransform(pospx, rotpx));

		actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
		actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD_FRICTION, true);
		//actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, true);

		coll->createShape();
		actor->attachShape(*coll->getShape());

		return actor;
	}
}
