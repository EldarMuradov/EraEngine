#pragma once

#include "physics_api.h"
#include "physics/physx_api.h"
#include "physics/core/physics.h"

#include <core/math.h>

#include <ecs/component.h>

namespace era_engine::physics
{

	enum class ArticulationCacheFlags
	{
		NONE = 0,
		VELOCITY = (1 << 0),
		ACCELERATION = (1 << 1),
		POSITION = (1 << 2),
		FORCE = (1 << 3),
		LINK_VELOCITY = (1 << 4),
		LINK_ACCELERATION = (1 << 5),
		ROOT_TRANSFORM = (1 << 6),
		ROOT_VELOCITIES = (1 << 7),
		ALL = (VELOCITY |
			ACCELERATION |
			POSITION |
			FORCE |
			LINK_VELOCITY |
			LINK_ACCELERATION |
			ROOT_TRANSFORM |
			ROOT_VELOCITIES)
	};

	class ERA_PHYSICS_API ArticulationComponent : public Component
	{
	public:
		struct ERA_PHYSICS_API ArticulationComponentDescriptor
		{
			vec3 root_position = vec3::zero;
			bool self_collision = true;
			bool fixed_base = false;
		};

		ArticulationComponent() = default;
		ArticulationComponent(ref<Entity::EcsData> _data, const ArticulationComponentDescriptor& _descriptor = {});
		~ArticulationComponent() override;

		void apply_cache(ArticulationCacheFlags flags = ArticulationCacheFlags::ALL, physx::PxArticulationCache* in_cache = nullptr);

		physx::PxArticulationCache* create_cache(ArticulationCacheFlags flags = ArticulationCacheFlags::ALL);

		ERA_VIRTUAL_REFLECT(Component)

	private:
		ArticulationComponentDescriptor descriptor;

		physx::PxArticulationReducedCoordinate* articulation = nullptr;
		physx::PxArticulationCache* cache = nullptr;

		std::vector<physx::PxArticulationJointReducedCoordinate*> joints;
		std::vector<physx::PxArticulationLink*> links;
	};

}