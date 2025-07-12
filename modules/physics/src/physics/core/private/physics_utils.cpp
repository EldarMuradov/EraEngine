#include "physics/core/physics_utils.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"

#include "ecs/world.h"
#include "ecs/base_components/transform_component.h"

namespace era_engine::physics
{
	physx::PxRigidDynamic* PhysicsUtils::create_rigid_dynamic(const physx::PxTransform& transform, void* user_data)
	{
		using namespace physx;

		PxRigidDynamic* actor = PhysicsHolder::physics_ref->get_physics()->createRigidDynamic(transform);
		ASSERT(actor != nullptr);

		actor->userData = user_data;

		return actor;
	}

	physx::PxRigidStatic* PhysicsUtils::create_rigid_static(const physx::PxTransform& transform, void* user_data)
	{
		using namespace physx;

		PxRigidStatic* actor = PhysicsHolder::physics_ref->get_physics()->createRigidStatic(transform);
		ASSERT(actor != nullptr);

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

		body_component->actor->setGlobalPose(PxTransform(create_PxVec3(pos), create_PxQuat(rot)));

		if (update_transform_component)
		{
			TransformComponent* transform_component = entity.get_component<TransformComponent>();
			transform_component->set_world_position(pos);
			transform_component->set_world_rotation(rot);
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

			if (body->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
			{
				return;
			}
			body->setAngularVelocity(PxVec3(0.0f));
			body->setLinearVelocity(PxVec3(0.0f));
		}
	}

	void PhysicsUtils::update_mass_and_inertia(DynamicBodyComponent* body_component, float density)
	{
		using namespace physx;

		if (PxRigidDynamic* body = body_component->get_rigid_dynamic())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			PxRigidBodyExt::updateMassAndInertia(*body, density);
		}
	}

}