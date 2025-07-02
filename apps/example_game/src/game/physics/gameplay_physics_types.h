#pragma once

#include <physics/collision_types.h>

namespace era_engine
{
	enum class GameCollisionType : uint32
	{
		TERRAIN = physics::CollisionType::COLLISION_0,
		DYNAMICS = physics::CollisionType::COLLISION_1,
		STATICS = physics::CollisionType::COLLISION_2
	};
}