#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

namespace era_engine::physics
{
	class ERA_PHYSICS_API PhysicsMaterial final
	{
	public:
		PhysicsMaterial(physx::PxPhysics* physics,
			float _restitution = 0.6f,
			float _static_friction = 0.8f,
			float _dynamic_friction = 0.8f);

		~PhysicsMaterial();

		void release();

		physx::PxMaterial* get_native_material() const;

		uint32 index = 0;
		std::string name;

	private:
		physx::PxMaterial* material = nullptr;
	};
}