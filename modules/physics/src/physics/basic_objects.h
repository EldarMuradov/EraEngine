#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include "core/math.h"

#include "ecs/component.h"

namespace era_engine::physics
{
	class ERA_PHYSICS_API PlaneComponent final : public Component
	{
	public:
		PlaneComponent() = default;
		PlaneComponent(ref<Entity::EcsData> _data, const vec3& _point, const vec3& _norm = vec3(0.0f, 1.0f, 0.0f));
		~PlaneComponent();

		virtual void release() override;

		ERA_VIRTUAL_REFLECT(Component)

	private:
		vec3 point = vec3();
		vec3 normal = vec3();
		physx::PxRigidStatic* plane = nullptr;
	};

	ERA_PHYSICS_API physx::PxRigidDynamic* create_rigid_cube(physx::PxReal half_extent, const physx::PxVec3& position);

	void create_cube(physx::PxArray<physx::PxVec3>& tri_verts, physx::PxArray<physx::PxU32>& tri_indices, const physx::PxVec3& pos, physx::PxReal scaling);
}