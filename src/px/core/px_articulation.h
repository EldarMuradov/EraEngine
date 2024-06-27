// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <px/core/px_physics_engine.h>

namespace physics
{
	using namespace physx;

	enum class px_articulation_cache_flags
	{
		articulation_cache_flag_none,
		articulation_cache_flag_velocity = (1 << 0),
		articulation_cache_flag_acceleration = (1 << 1),
		articulation_cache_flag_position = (1 << 2),
		articulation_cache_flag_force = (1 << 3),
		articulation_cache_flag_link_velocity = (1 << 4),
		articulation_cache_flag_link_acceleration = (1 << 5),
		articulation_cache_flag_root_transform = (1 << 6),
		articulation_cache_flag_root_velocities = (1 << 7),
		articulation_cache_flag_all = (articulation_cache_flag_velocity |
			articulation_cache_flag_acceleration | 
			articulation_cache_flag_position |
			articulation_cache_flag_force |
			articulation_cache_flag_link_velocity |
			articulation_cache_flag_link_acceleration |
			articulation_cache_flag_root_transform |
			articulation_cache_flag_root_velocities)
	};

	struct px_articulation
	{
		px_articulation(const vec3& rootPose = vec3(0.0f), bool fixedBase = true, bool selfCollision = true) noexcept;

		virtual ~px_articulation()
		{
			physics_holder::physicsRef->getScene()->removeArticulation(*articulation);

			joints.clear();
			links.clear();

			PX_RELEASE(articulation)
			PX_RELEASE(cache)
			PX_RELEASE(material)
		}

		PxArticulationCache* createCache(px_articulation_cache_flags flags = px_articulation_cache_flags::articulation_cache_flag_all) noexcept
		{
			PX_RELEASE(cache)

			cache = articulation->createCache();
			articulation->copyInternalStateToCache(*cache, (PxArticulationCacheFlags)((uint32)flags));
			return cache;
		}

		void applyCache(px_articulation_cache_flags flags = px_articulation_cache_flags::articulation_cache_flag_all, PxArticulationCache* inCache = nullptr)
		{
			if (inCache)
				articulation->applyCache(*inCache, (PxArticulationCacheFlags)((uint32)flags));
			else
				articulation->applyCache(*cache, (PxArticulationCacheFlags)((uint32)flags));
		}

	protected:
		PxArticulationReducedCoordinate* articulation = nullptr;

		PxArticulationCache* cache = nullptr;

		PxMaterial* material = nullptr;

		std::vector<PxArticulationJointReducedCoordinate*> joints;
		std::vector<PxArticulationLink*> links;
	};
}