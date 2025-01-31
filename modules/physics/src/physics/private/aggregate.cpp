#include "physics/aggregate.h"
#include "physics/core/physics.h"

namespace era_engine::physics
{
	Aggregate::Aggregate(uint8_t nb, bool sc)
		: nb_actors(nb), self_collisions(sc)
	{
		using namespace physx;

		auto& physics_ref = PhysicsHolder::physics_ref;

		aggregate = physics_ref->get_physics()->createAggregate(nb_actors, nb_actors, PxGetAggregateFilterHint(PxAggregateType::eGENERIC, self_collisions));
		physics_ref->get_scene()->addAggregate(*aggregate);
	}

	Aggregate::~Aggregate()
	{
		PhysicsHolder::physics_ref->get_scene()->removeAggregate(*aggregate);
		PX_RELEASE(aggregate)
	}

	void Aggregate::add_actor(physx::PxActor* actor)
	{
		aggregate->addActor(*actor);
	}

	void Aggregate::remove_actor(physx::PxActor* actor)
	{
		aggregate->removeActor(*actor);
	}

	uint8_t Aggregate::get_nb_actors() const noexcept
	{
		return nb_actors;
	}

	bool Aggregate::is_self_collision() const noexcept
	{
		return self_collisions;
	}
}