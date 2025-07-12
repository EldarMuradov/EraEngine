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
		PhysicsHolder::physics_ref->remove_actor(this);

		PX_RELEASE(actor)
	}

	physx::PxRigidActor* BodyComponent::get_rigid_actor() const
	{
		return actor;
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
		mass_space_inertia_tensor.set_component(ComponentPtr(this));
	}

	DynamicBodyComponent::~DynamicBodyComponent()
	{
	}

	physx::PxRigidDynamic* DynamicBodyComponent::get_rigid_dynamic() const
	{
		return actor == nullptr ? nullptr : actor->is<physx::PxRigidDynamic>();
	}

	vec3 DynamicBodyComponent::get_physics_position() const
	{
		using namespace physx;

		PxSceneReadLock lock(*PhysicsHolder::physics_ref->get_scene());
		const PxVec3 pos = actor->getGlobalPose().p;
		return create_vec3(pos);
	}

	void DynamicBodyComponent::wake_up()
	{
		using namespace physx;

		if (actor == nullptr)
		{
			return;
		}

		if (PxRigidDynamic* body = actor->is<PxRigidDynamic>())
		{
			body->wakeUp();
		}
	}

	StaticBodyComponent::StaticBodyComponent(ref<Entity::EcsData> _data)
		: BodyComponent(_data)
	{
		simulated.set_component(ComponentPtr(this));
	}

	StaticBodyComponent::~StaticBodyComponent()
	{
	}

	physx::PxRigidStatic* StaticBodyComponent::get_rigid_static() const
	{
		return actor->is<physx::PxRigidStatic>();
	}

}