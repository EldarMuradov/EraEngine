#pragma once

#include "physics/physx_api.h"

#include "core/math.h"

#include "ecs/component.h"

namespace era_engine::physics
{
	class PlaneComponent final : public Component
	{
	public:
		PlaneComponent() = default;
		PlaneComponent(ref<Entity::EcsData> _data, const vec3& _point, const vec3& _norm);
		~PlaneComponent();

		virtual void release() override;

		ERA_VIRTUAL_REFLECT(Component)

	private:
		vec3 point = vec3();
		vec3 normal = vec3();
		physx::PxRigidStatic* plane = nullptr;
	};
}