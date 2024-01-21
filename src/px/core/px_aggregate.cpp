#include "pch.h"
#include "px/core/px_aggregate.h"

px_aggregate::px_aggregate(uint8 nb, bool sc) noexcept : nbActors(nb), selfCollisions(sc)
{
	aggregate = px_physics_engine::getPhysics()->createAggregate(nbActors, selfCollisions, PxAggregateFilterHint());
	px_physics_engine::get()->getPhysicsAdapter()->scene->addAggregate(*aggregate);
}

void px_aggregate::addActor(physx::PxActor* actor) noexcept
{
	aggregate->addActor(*actor);
}

void px_aggregate::removeActor(physx::PxActor* actor) noexcept
{
	aggregate->removeActor(*actor);
}
