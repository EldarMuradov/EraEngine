// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "px/core/px_physics_engine.h"

namespace era_engine::physics
{
	using namespace physx;

	struct px_aggregate
	{
		px_aggregate() = default;
		px_aggregate(uint8 nb, bool sc = true) noexcept;
		~px_aggregate() { physics_holder::physicsRef->getScene()->removeAggregate(*aggregate); PX_RELEASE(aggregate) }

		void addActor(PxActor* actor) noexcept;
		void removeActor(PxActor* actor) noexcept;

		uint8 getNbActors() const noexcept { return nbActors; }
		bool isSelfCollision() const noexcept { return selfCollisions; }

	private:
		PxAggregate* aggregate = nullptr;

		uint8 nbActors = 0;
		bool selfCollisions = true;
	};
}