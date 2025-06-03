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

		PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());

		body_component->actor->setGlobalPose(PxTransform(create_PxVec3(pos), create_PxQuat(rot)));

		if (update_transform_component)
		{
			TransformComponent& transform_component = entity.get_component<TransformComponent>();
			transform_component.transform.position = pos;
			transform_component.transform.rotation = rot;
		}

		if (auto dyn = body_component->actor->is<PxRigidDynamic>())
		{
			if (dyn->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
			{
				return;
			}
			dyn->setAngularVelocity(PxVec3(0.0f));
			dyn->setLinearVelocity(PxVec3(0.0f));
		}
	}

	void PhysicsUtils::manual_set_physics_transform(Entity entity, const trs& transform, bool update_transform_component)
	{
		manual_set_physics_transform(entity, transform.position, transform.rotation, update_transform_component);
	}

}