// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "px/core/px_physics_engine.h"

namespace era_engine::physics
{
	using namespace physx;

	struct px_aggregate
	{
		px_aggregate() = default;
		px_aggregate(uint8 nb, bool sc = true);
		~px_aggregate() { physics_holder::physicsRef->getScene()->removeAggregate(*aggregate); PX_RELEASE(aggregate) }

		void addActor(PxActor* actor);
		void removeActor(PxActor* actor);

		uint8 getNbActors() const { return nbActors; }
		bool isSelfCollision() const { return selfCollisions; }

	private:
		PxAggregate* aggregate = nullptr;

		uint8 nbActors = 0;
		bool selfCollisions = true;
	};
}