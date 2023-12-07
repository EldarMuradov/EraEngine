#pragma once
#include "px/core/px_physics_engine.h"

struct px_aggregate 
{
	px_aggregate() = default;
	px_aggregate(uint8 nb, bool sc = true) noexcept;
	~px_aggregate() { px_physics_engine::get()->getPhysicsAdapter()->scene->removeAggregate(*aggregate); PX_RELEASE(aggregate) }

	void addActor(physx::PxActor* actor) noexcept;
	void removeActor(physx::PxActor* actor) noexcept;

	uint8 getNbActors() const noexcept { return nbActors; }
	bool isSelfCollision() const noexcept { return selfCollisions; }

private:
	physx::PxAggregate* aggregate = nullptr;

	uint8 nbActors = 0;
	bool selfCollisions = true;
};