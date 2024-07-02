// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "pch.h"
#include "px/core/px_aggregate.h"

namespace era_engine
{
	physics::px_aggregate::px_aggregate(uint8 nb, bool sc) noexcept : nbActors(nb), selfCollisions(sc)
	{
		aggregate = physics_holder::physicsRef->getPhysics()->createAggregate(nbActors, selfCollisions, PxAggregateFilterHint());
		physics_holder::physicsRef->getScene()->addAggregate(*aggregate);
	}

	void physics::px_aggregate::addActor(PxActor* actor) noexcept
	{
		aggregate->addActor(*actor);
	}

	void physics::px_aggregate::removeActor(PxActor* actor) noexcept
	{
		aggregate->removeActor(*actor);
	}

}