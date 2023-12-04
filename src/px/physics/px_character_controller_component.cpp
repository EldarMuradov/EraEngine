#include "pch.h"
#include "px/physics/px_character_controller_component.h"
#include <scene/scene.h>

px_character_controller_component::px_character_controller_component(eentity* entt) noexcept : entity(entt)
{
	transform = entity->getComponentIfExists<transform_component>();
	createCharacterController();
}

px_character_controller_component::px_character_controller_component(eentity* entt, float h, float r, float m) noexcept : entity(entt), height(h), radius(r), mass(m)
{
	transform = entity->getComponentIfExists<transform_component>();
	createCharacterController();
}

void px_character_controller_component::createCharacterController() noexcept
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
