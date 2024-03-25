#pragma once
#include <px/core/px_physics_engine.h>

namespace physics
{
	using namespace physx;

	enum class px_articulation_cache_flags
	{
		None,
		Velocity = (1 << 0),
		Acceleration = (1 << 1),
		Position = (1 << 2),
		Force = (1 << 3),
		LinkVelocity = (1 << 4),
		LinkAcceleration = (1 << 5),
		RootTransform = (1 << 6),
		RootVelocities = (1 << 7),
		All = (Velocity | Acceleration | Position | LinkVelocity | LinkAcceleration | RootTransform | RootVelocities)
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

		PxArticulationCache* createCache(px_articulation_cache_flags flags = px_articulation_cache_flags::All) noexcept
		{
			PX_RELEASE(cache)

			cache = articulation->createCache();
			articulation->copyInternalStateToCache(*cache, (PxArticulationCacheFlags)((uint32)flags));
			return cache;
		}

		void applyCache(px_articulation_cache_flags flags = px_articulation_cache_flags::All, PxArticulationCache* inCache = nullptr)
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

		::std::vector<PxArticulationJointReducedCoordinate*> joints;
		::std::vector<PxArticulationLink*> links;
	};
}