#include "physics/core/physics_utils.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"

#include "ecs/world.h"

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

}