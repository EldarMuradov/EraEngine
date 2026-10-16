#include "physics/collision_types.h"
#include "physics/body_component.h"
#include "physics/physical_animation/ragdoll_profiles_asset.h"

#include <ecs/serialization/value_adapters.h>

#include <asset/asset_type_registry.h>

#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::enumeration<CollisionType>("CollisionType")
		(
			value("NONE", CollisionType::NONE),
			value("COLLISION_0", CollisionType::COLLISION_0),
			value("COLLISION_1", CollisionType::COLLISION_1),
			value("COLLISION_2", CollisionType::COLLISION_2),
			value("COLLISION_3", CollisionType::COLLISION_3),
			value("COLLISION_4", CollisionType::COLLISION_4),
			value("COLLISION_5", CollisionType::COLLISION_5),
			value("COLLISION_6", CollisionType::COLLISION_6),
			value("COLLISION_7", CollisionType::COLLISION_7),
			value("COLLISION_8", CollisionType::COLLISION_8),
			value("COLLISION_9", CollisionType::COLLISION_9),
			value("COLLISION_10", CollisionType::COLLISION_10),
			value("COLLISION_11", CollisionType::COLLISION_11),
			value("COLLISION_12", CollisionType::COLLISION_12),
			value("COLLISION_13", CollisionType::COLLISION_13),
			value("COLLISION_14", CollisionType::COLLISION_14),
			value("COLLISION_15", CollisionType::COLLISION_15),
			value("COLLISION_16", CollisionType::COLLISION_16),
			value("COLLISION_17", CollisionType::COLLISION_17),
			value("COLLISION_18", CollisionType::COLLISION_18),
			value("COLLISION_19", CollisionType::COLLISION_19),
			value("COLLISION_20", CollisionType::COLLISION_20),
			value("COLLISION_21", CollisionType::COLLISION_21),
			value("COLLISION_22", CollisionType::COLLISION_22),
			value("COLLISION_23", CollisionType::COLLISION_23),
			value("COLLISION_24", CollisionType::COLLISION_24),
			value("COLLISION_25", CollisionType::COLLISION_25),
			value("COLLISION_26", CollisionType::COLLISION_26),
			value("COLLISION_27", CollisionType::COLLISION_27),
			value("COLLISION_28", CollisionType::COLLISION_28),
			value("VEHICLE", CollisionType::VEHICLE),
			value("TERRAIN", CollisionType::TERRAIN),
			value("RAGDOLL", CollisionType::RAGDOLL),
			value("ALL", CollisionType::ALL)
		);
		ValueAdapters::register_enum_converter<CollisionType>();
		ValueAdapters::register_observable<CollisionType>();

		registration::enumeration<KinematicMotionType>("KinematicMotionType")
		(
			value("TELEPORT", KinematicMotionType::TELEPORT),
			value("VELOCITY", KinematicMotionType::VELOCITY)
		);
		ValueAdapters::register_observable<KinematicMotionType>();

		AssetTypeRegistry::register_asset_type<RagdollProfilesAsset>();
	}
}
