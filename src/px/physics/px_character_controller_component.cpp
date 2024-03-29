// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"
#include <px/physics/px_rigidbody_component.h>
#include "px/physics/px_character_controller_component.h"
#include <scene/scene.h>
#include <application.h>

namespace physics
{
	px_capsule_cct_component::px_capsule_cct_component(uint32_t entt) noexcept : px_cct_component_base(entt)
	{
		type = px_cct_type::Capsule;
		createCharacterController();
		uint32_t* h = new uint32_t[1];
		h[0] = handle;
		controller->getActor()->userData = h;
		physics_holder::physicsRef->addActor(this, controller->getActor(), false);
	}

	void px_capsule_cct_component::createCharacterController() noexcept
	{
		manager = PxCreateControllerManager(*physics_holder::physicsRef->getScene());

		material = physics_holder::physicsRef->getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);

		eentity entity = { handle, &physics_holder::physicsRef->app.getCurrentScene()->registry };

		PxCapsuleControllerDesc desc;

		desc.height = height;
		desc.material = material;
		desc.maxJumpHeight = height * 0.5f;
		desc.radius = radius;
		desc.slopeLimit = cosf(deg2rad(45.0f));
		desc.invisibleWallHeight = height * 0.5f;
		auto trsPos = entity.getComponent<transform_component>().position;
		PxExtendedVec3 pos = PxExtendedVec3(trsPos.x, trsPos.y, trsPos.z);
		desc.position = pos;
		desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
		uint32_t* h = new uint32_t[1];
		h[0] = handle;
		desc.userData = h;
		controller = manager->createController(desc);

		controller->getActor()->setMass(mass);
		controller->getActor()->setRigidBodyFlags(physx::PxRigidBodyFlag::eKINEMATIC);
	}

	px_capsule_cct_component::px_capsule_cct_component(uint32_t entt, float h, float r, float m) noexcept : px_cct_component_base(entt), height(h), radius(r)
	{
		mass = m;
		type = px_cct_type::Capsule;
		createCharacterController();
		uint32_t* hndl = new uint32_t[1];
		hndl[0] = handle;
		controller->getActor()->userData = hndl;
		physics_holder::physicsRef->addActor(this, controller->getActor(), false);
	}

	px_cct_component_base::px_cct_component_base(uint32_t entt) noexcept
	{
		handle = entt;
	}

	void px_cct_component_base::release(bool releaseActor) noexcept
	{
		physics_holder::physicsRef->removeActor(this);

		PX_RELEASE(controller)
		PX_RELEASE(manager)
		PX_RELEASE(material)
		if(releaseActor)
			PX_RELEASE(actor)
	}

	px_box_cct_component::px_box_cct_component(uint32_t entt) noexcept : px_cct_component_base(entt)
	{
		type = px_cct_type::Box;
		createCharacterController();
		uint32_t* h = new uint32_t[1];
		h[0] = handle;
		controller->getActor()->userData = h;
		physics_holder::physicsRef->addActor(this, controller->getActor(), false);
	}

	px_box_cct_component::px_box_cct_component(uint32_t entt, float hh, float hs, float m) noexcept : px_cct_component_base(entt)
	{
		mass = m;
		type = px_cct_type::Box;
		createCharacterController();
		uint32_t* h = new uint32_t[1];
		h[0] = handle;
		controller->getActor()->userData = h;
		physics_holder::physicsRef->addActor(this, controller->getActor(), false);
	}

	void px_box_cct_component::createCharacterController() noexcept
	{
		manager = PxCreateControllerManager(*physics_holder::physicsRef->getScene());

		material = physics_holder::physicsRef->getPhysics()->createMaterial(0.5f, 0.5f, 0.6f);
		eentity entity = { handle, &physics_holder::physicsRef->app.getCurrentScene()->registry };

		physx::PxBoxControllerDesc desc;

		desc.halfHeight = halfHeight;
		desc.material = material;
		desc.maxJumpHeight = halfHeight;
		desc.halfSideExtent = halfSideExtent;
		desc.slopeLimit = cosf(deg2rad(45.0f));
		desc.invisibleWallHeight = halfHeight;
		auto trsPos = entity.getComponent<transform_component>().position;
		physx::PxExtendedVec3 pos = physx::PxExtendedVec3(trsPos.x, trsPos.y, trsPos.z);
		desc.position = pos;
		uint32_t* h = new uint32_t[1];
		h[0] = handle;
		desc.userData = h;
		controller = manager->createController(desc);

		controller->getActor()->setMass(mass);
		controller->getActor()->setRigidBodyFlags(physx::PxRigidBodyFlag::eKINEMATIC);
	}
}