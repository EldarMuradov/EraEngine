#include "pch.h"
#include "px/physics/px_character_controller_component.h"
#include <scene/scene.h>

px_capsule_cct_component::px_capsule_cct_component(eentity* entt) noexcept : px_cct_component_base(entt)
{
	type = px_cct_type::px_capsule;
	createCharacterController();
}

void px_capsule_cct_component::createCharacterController() noexcept
{
	manager = PxCreateControllerManager(*px_physics_engine::get()->getPhysicsAdapter()->scene);

	material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);

	physx::PxCapsuleControllerDesc desc;

	desc.height = height;
	desc.material = material;
	desc.maxJumpHeight = height * 0.5f;
	desc.radius = radius;
	desc.slopeLimit = cosf(deg2rad(45.0f));
	desc.invisibleWallHeight = height * 0.5f;
	auto trsPos = transform->position;
	physx::PxExtendedVec3 pos = physx::PxExtendedVec3(trsPos.x, trsPos.y, trsPos.z);
	desc.position = pos;
	desc.climbingMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;
	uint32_t* handle = new uint32_t[1];
	handle[0] = (uint32_t)entity->handle;
	desc.userData = handle;
	controller = manager->createController(desc);
	
	controller->getActor()->setMass(mass);
	controller->getActor()->setRigidBodyFlags(physx::PxRigidBodyFlag::eKINEMATIC);
}

px_capsule_cct_component::px_capsule_cct_component(eentity* entt, float h, float r, float m) noexcept : px_cct_component_base(entt), height(h), radius(r)
{
	mass = m;
	type = px_cct_type::px_capsule;
	createCharacterController();
}

px_cct_component_base::px_cct_component_base(eentity* entt) noexcept
{
	entity = entt;
	transform = entity->getComponentIfExists<transform_component>();
}

px_box_cct_component::px_box_cct_component(eentity* entt) noexcept : px_cct_component_base(entt)
{
	type = px_cct_type::px_box;
	createCharacterController();
}

px_box_cct_component::px_box_cct_component(eentity* entt, float hh, float hs, float m) noexcept : px_cct_component_base(entt)
{
	mass = m;
	type = px_cct_type::px_box;
	createCharacterController();
}

void px_box_cct_component::createCharacterController() noexcept
{
	manager = PxCreateControllerManager(*px_physics_engine::get()->getPhysicsAdapter()->scene);

	material = px_physics_engine::getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);

	physx::PxBoxControllerDesc desc;

	desc.halfHeight = halfHeight;
	desc.material = material;
	desc.maxJumpHeight = halfHeight;
	desc.halfSideExtent = halfSideExtent;
	desc.slopeLimit = cosf(deg2rad(45.0f));
	desc.invisibleWallHeight = halfHeight;
	auto trsPos = transform->position;
	physx::PxExtendedVec3 pos = physx::PxExtendedVec3(trsPos.x, trsPos.y, trsPos.z);
	desc.position = pos;
	uint32_t* handle = new uint32_t[1];
	handle[0] = (uint32_t)entity->handle;
	desc.userData = handle;
	controller = manager->createController(desc);

	controller->getActor()->setMass(mass);
	controller->getActor()->setRigidBodyFlags(physx::PxRigidBodyFlag::eKINEMATIC);
}
