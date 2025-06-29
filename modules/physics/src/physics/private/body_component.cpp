// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/shape_component.h"
#include "physics/cct_component.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"

#include "ecs/world.h"
#include "ecs/base_components/transform_component.h"

#include <rttr/registration>

namespace era_engine::physics
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<BodyComponent>("BodyComponent")
			.constructor<>();

		registration::class_<DynamicBodyComponent>("DynamicBodyComponent")
			.constructor<>();

		registration::class_<StaticBodyComponent>("StaticBodyComponent")
			.constructor<>();
	}

	BodyComponent::BodyComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	BodyComponent::~BodyComponent()
	{
	}

	physx::PxRigidActor* BodyComponent::get_rigid_actor() const
	{
		return actor;
	}

	uint32_t BodyComponent::get_filter_mask() const
	{
		return filter_mask;
	}

	void BodyComponent::set_filter_mask(uint32_t group, uint32_t mask)
	{
		filter_mask = mask;
		filter_group = group;
		setup_filter_mask();
	}

	void BodyComponent::release()
	{
		PhysicsHolder::physics_ref->remove_actor(this);
		PX_RELEASE(actor)
		Component::release();
	}

	void BodyComponent::setup_filter_mask()
	{
		using namespace physx;

		auto& colliders = PhysicsHolder::physics_ref->colliders_map[component_data->entity_handle];

		for (auto& collider : colliders)
		{
			ShapeUtils::setup_filtering(collider->get_shape(), filter_group, filter_mask);
		}
	}

	void BodyComponent::detach_shape(physx::PxShape* shape)
	{
		if (actor != nullptr)
		{
			actor->detachShape(*shape);
		}
	}

	DynamicBodyComponent::DynamicBodyComponent(ref<Entity::EcsData> _data)
		: BodyComponent(_data)
	{
		simulated.set_component(ComponentPtr(this));
		kinematic.set_component(ComponentPtr(this));
		ccd.set_component(ComponentPtr(this));
		use_gravity.set_component(ComponentPtr(this));
		constraints.set_component(ComponentPtr(this));
		linear_damping.set_component(ComponentPtr(this));
		angular_damping.set_component(ComponentPtr(this));
		linear_velocity.set_component(ComponentPtr(this));
		angular_velocity.set_component(ComponentPtr(this));
		center_of_mass.set_component(ComponentPtr(this));
		mass.set_component(ComponentPtr(this));
		max_angular_velocity.set_component(ComponentPtr(this));
		max_linear_velocity.set_component(ComponentPtr(this));
		max_contact_impulse.set_component(ComponentPtr(this));
		max_depenetration_velocity.set_component(ComponentPtr(this));
		sleep_threshold.set_component(ComponentPtr(this));
		stabilization_threshold.set_component(ComponentPtr(this));
		solver_velocity_iterations_count.set_component(ComponentPtr(this));
		solver_position_iterations_count.set_component(ComponentPtr(this));
	}

	DynamicBodyComponent::~DynamicBodyComponent()
	{
	}

	physx::PxRigidDynamic* DynamicBodyComponent::get_rigid_dynamic() const
	{
		return actor == nullptr ? nullptr : actor->is<physx::PxRigidDynamic>();
	}

	void DynamicBodyComponent::set_mass_space_inertia_tensor(const vec3& tensor)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setMassSpaceInertiaTensor(create_PxVec3(tensor));
		}
	}

	void DynamicBodyComponent::update_mass_and_inertia(float density)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			PxRigidBodyExt::updateMassAndInertia(*dyn, density);
		}
	}

	vec3 DynamicBodyComponent::get_physics_position() const
	{
		using namespace physx;

		PxSceneReadLock lock(*PhysicsHolder::physics_ref->get_scene());
		PxVec3 pos = actor->getGlobalPose().p;
		return create_vec3(pos);
	}

	void DynamicBodyComponent::manual_clear_force_and_torque()
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->clearForce();
			dyn->clearTorque();

			if (dyn->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
			{
				return;
			}
			dyn->setAngularVelocity(PxVec3(0.0f));
			dyn->setLinearVelocity(PxVec3(0.0f));
		}
	}

	StaticBodyComponent::StaticBodyComponent(ref<Entity::EcsData> _data)
		: BodyComponent(_data)
	{
	}

	StaticBodyComponent::~StaticBodyComponent()
	{
	}

	physx::PxRigidStatic* StaticBodyComponent::get_rigid_static() const
	{
		return actor->is<physx::PxRigidStatic>();
	}

}