#include "physics/core/physics_utils.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"

#include <ecs/world.h>
#include <ecs/base_components/transform_component.h>

#include <core/log.h>

namespace era_engine::physics
{
	physx::PxRigidDynamic* PhysicsUtils::create_rigid_dynamic(const physx::PxTransform& transform, void* user_data)
	{
		using namespace physx;

		PxRigidDynamic* actor = PhysicsHolder::physics_ref->get_physics()->createRigidDynamic(transform);
		ASSERT(actor != nullptr);
		
#ifndef _DEBUG
		actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
#endif

		actor->userData = user_data;

		return actor;
	}

	physx::PxRigidStatic* PhysicsUtils::create_rigid_static(const physx::PxTransform& transform, void* user_data)
	{
		using namespace physx;

		PxRigidStatic* actor = PhysicsHolder::physics_ref->get_physics()->createRigidStatic(transform);
		ASSERT(actor != nullptr);

#ifndef _DEBUG
		actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
#endif

		actor->userData = user_data;

		return actor;
	}

	BodyComponent* PhysicsUtils::get_body_component(ref<Entity::EcsData> entity_data)
	{
		return get_body_component(Entity(entity_data));
	}

	BodyComponent* PhysicsUtils::get_body_component(weakref<Entity::EcsData> entity_data)
	{
		if (entity_data.expired())
		{
			return nullptr;
		}

		return get_body_component(Entity(entity_data.lock()));
	}

	BodyComponent* PhysicsUtils::get_body_component(Entity entity)
	{
		if (DynamicBodyComponent* body_component = entity.get_component_if_exists<DynamicBodyComponent>())
		{
			return body_component;
		}
		else if (StaticBodyComponent* body_component = entity.get_component_if_exists<StaticBodyComponent>())
		{
			return body_component;
		}

		return nullptr;
	}

	void PhysicsUtils::manual_update_mass(DynamicBodyComponent* dynamic_body_component)
	{
		using namespace physx;

		if (dynamic_body_component == nullptr)
		{
			return;
		}

		if (dynamic_body_component->actor == nullptr)
		{
			return;
		}

		PxRigidDynamic* rigid_dynamic = dynamic_body_component->actor->is<PxRigidDynamic>();
		if (rigid_dynamic == nullptr)
		{
			return;
		}

		const bool body_is_kinematic = dynamic_body_component->kinematic;

		PxRigidBody* physx_rigid_body = dynamic_body_component->actor->is<PxRigidBody>();
		ASSERT(physx_rigid_body != nullptr);

		if (!body_is_kinematic)
		{
			PxVec3 center_of_mass = create_PxVec3(dynamic_body_component->center_of_mass);
			PxVec3* center_of_mass_ptr = &center_of_mass;

			const bool result = PxRigidBodyExt::setMassAndUpdateInertia(
				*physx_rigid_body,
				dynamic_body_component->mass.get(),
				center_of_mass_ptr,
				false);

			if (!result)
			{
				LOG_ERROR("Could not update mass properties for entity!");
			}
		}

		dynamic_body_component->center_of_mass.get_silent_for_write() = create_vec3(physx_rigid_body->getCMassLocalPose().p);
		dynamic_body_component->mass_space_inertia_tensor.get_silent_for_write() = create_vec3(physx_rigid_body->getMassSpaceInertiaTensor());
	}

	void PhysicsUtils::manual_set_physics_transform(Entity entity, const vec3& pos, const quat& rot, bool update_transform_component)
	{
		using namespace physx;

		BodyComponent* body_component = get_body_component(entity);

		if (body_component == nullptr)
		{
			return;
		}

		if(body_component->actor == nullptr)
		{
			return;
		}

		PxTransform physics_transform = PxTransform(create_PxVec3(pos), create_PxQuat(rot));

		PxRigidDynamic* rigid_dynamic = body_component->actor->is<PxRigidDynamic>();
		if (!body_component->actor->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION) &&
			rigid_dynamic != nullptr &&
			rigid_dynamic->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC) &&
			dynamic_cast<DynamicBodyComponent*>(body_component)->kinematic_motion_type.get() == KinematicMotionType::VELOCITY)
		{
			rigid_dynamic->setKinematicTarget(physics_transform);
		}
		else
		{
			body_component->actor->setGlobalPose(physics_transform);
		}

		if (update_transform_component)
		{
			TransformComponent* transform_component = entity.get_component<TransformComponent>();
			transform_component->set_world_transform(trs(pos, rot, transform_component->get_world_transform().scale));
		}
	}

	void PhysicsUtils::manual_set_physics_transform(Entity entity, const trs& transform, bool update_transform_component)
	{
		manual_set_physics_transform(entity, transform.position, transform.rotation, update_transform_component);
	}

	void PhysicsUtils::manual_clear_force_and_torque(DynamicBodyComponent* body_component)
	{
		using namespace physx;

		if (PxRigidDynamic* body = body_component->get_rigid_dynamic())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			body->clearForce();
			body->clearTorque();

			if (body_component->kinematic)
			{
				return;
			}
			body_component->linear_velocity = vec3::zero;
			body_component->angular_velocity = vec3::zero;
		}
	}

	void PhysicsUtils::update_mass_and_inertia(DynamicBodyComponent* body_component, float density)
	{
		using namespace physx;

		if (PxRigidDynamic* body = body_component->get_rigid_dynamic())
		{
			const PxVec3 center_of_mass = create_PxVec3(body_component->center_of_mass);

			PxRigidBodyExt::updateMassAndInertia(*body, density, &center_of_mass);
		}
	}

}