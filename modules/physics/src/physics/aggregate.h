#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include "ecs/reflection.h"

namespace era_engine::physics
{
	class ERA_PHYSICS_API Aggregate
	{
	public:
		Aggregate() = default;
		Aggregate(uint8_t nb, bool sc = true);
		virtual ~Aggregate();

		void add_actor(physx::PxActor* actor);
		void remove_actor(physx::PxActor* actor);

		uint8_t get_nb_actors() const noexcept;
		bool is_self_collision() const noexcept;

		ERA_REFLECT

	private:
		physx::PxAggregate* aggregate = nullptr;

		uint8_t nb_actors = 0;
		bool self_collisions = true;
	};
}