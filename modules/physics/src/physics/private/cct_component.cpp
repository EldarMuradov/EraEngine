// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "physics/cct_component.h"
#include "physics/core/physics.h"

#include "ecs/world.h"
#include "ecs/base_components/transform_component.h"

#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<CCTBaseComponent>("CCTBaseComponent")
			.constructor<>();

		rttr::registration::class_<BoxCCTComponent>("BoxCCTComponent")
			.constructor<>();

		rttr::registration::class_<CapsuleCCTComponent>("CapsuleCCTComponent")
			.constructor<>();
	}

	CCTBaseComponent::CCTBaseComponent(ref<Entity::EcsData> _data, float _mass)
		: BodyComponent(_data), mass(_mass)
	{
		
	}

	CCTBaseComponent::~CCTBaseComponent()
	{
	}

	void CCTBaseComponent::create_character_controller()
	{
	}

	void CCTBaseComponent::register_cct()
	{
		controller->getActor()->userData = static_cast<void*>(component_data.get());

		PhysicsHolder::physics_ref->add_actor((BodyComponent*)this, controller->getActor());
	}

	void CCTBaseComponent::release()
	{
		PX_RELEASE(controller)
		PX_RELEASE(manager)

		BodyComponent::release();
	}

	BoxCCTComponent::BoxCCTComponent(ref<Entity::EcsData> _data, float _half_height, float _half_side_extent, float _mass)
		: CCTBaseComponent(_data, _mass)
	{
		create_character_controller();
		register_cct();
	}

	BoxCCTComponent::~BoxCCTComponent()
	{
	}

	void BoxCCTComponent::create_character_controller()
	{
		using namespace physx;
		manager = PxCreateControllerManager(*PhysicsHolder::physics_ref->get_scene());

		Entity entity = get_world()->get_entity(component_data->entity_handle);
		const TransformComponent* transform = entity.get_component_if_exists<TransformComponent>();

		PxBoxControllerDesc desc;

		desc.halfHeight = half_height;
		desc.material = PhysicsHolder::physics_ref->get_default_material();
		desc.maxJumpHeight = half_height;
		desc.halfSideExtent = half_side_extent;
		desc.slopeLimit = cosf(deg2rad(45.0f));
		desc.invisibleWallHeight = half_height;
		desc.position = PxExtendedVec3(transform->transform.position.x, transform->transform.position.y, transform->transform.position.z);
		controller = manager->createController(desc);

		controller->getActor()->setMass(mass);
		controller->getActor()->setRigidBodyFlags(physx::PxRigidBodyFlag::eKINEMATIC);
	}

	CapsuleCCTComponent::CapsuleCCTComponent(ref<Entity::EcsData> _data, float _height, float _radius, float _mass)
		: CCTBaseComponent(_data, _mass), height(_height), radius(_radius)
	{
		create_character_controller();
		register_cct();
	}

	CapsuleCCTComponent::~CapsuleCCTComponent()
	{
	}

	void CapsuleCCTComponent::create_character_controller()
	{
		using namespace physx;

		manager = PxCreateControllerManager(*PhysicsHolder::physics_ref->get_scene());

		Entity entity = get_world()->get_entity(component_data->entity_handle);
		const TransformComponent* transform = entity.get_component_if_exists<TransformComponent>();

		PxCapsuleControllerDesc desc;
		desc.height = height;
		desc.material = PhysicsHolder::physics_ref->get_default_material();
		desc.maxJumpHeight = height * 0.5f;
		desc.radius = radius;
		desc.slopeLimit = cosf(deg2rad(45.0f));
		desc.invisibleWallHeight = height * 0.5f;
		desc.position = PxExtendedVec3(transform->transform.position.x, transform->transform.position.y, transform->transform.position.z);
		desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
		controller = manager->createController(desc);

		controller->getActor()->setMass(mass);
		controller->getActor()->setRigidBodyFlags(physx::PxRigidBodyFlag::eKINEMATIC);
	}

}