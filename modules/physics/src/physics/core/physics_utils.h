#pragma once

#include "physics/physx_api.h"

#include <core/math.h>

#include <ecs/entity.h>

namespace era_engine::physics
{
	class BodyComponent;

	class PhysicsUtils final
	{
		PhysicsUtils() = delete;
	public:
		static physx::PxRigidDynamic* create_rigid_dynamic(const physx::PxTransform& transform, void* user_data);
		static physx::PxRigidStatic* create_rigid_static(const physx::PxTransform& transform, void* user_data);

		static BodyComponent* get_body_component(ref<Entity::EcsData> entity_data);
		static BodyComponent* get_body_component(weakref<Entity::EcsData> entity_data);
		static BodyComponent* get_body_component(Entity entity);

		static void manual_set_physics_transform(Entity entity, const vec3& pos, const quat& rot, bool update_transform_component = true);
		static void manual_set_physics_transform(Entity entity, const trs& transform, bool update_transform_component = true);
	};
}