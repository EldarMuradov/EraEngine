#pragma once

#include "physics_api.h"

#include "physics/destructions/destruction_types.h"

#include <core/math.h>

#include <ecs/component.h>

namespace era_engine::physics
{
	class ERA_PHYSICS_API DestructibleComponent : public Component
	{
	public:
		DestructibleComponent() = default;
		DestructibleComponent(ref<Entity::EcsData> _data, uint32 _health = 100, bool _is_root = true);
		~DestructibleComponent() override;

	protected:
		ref<DestructibleAsset> blast_asset = nullptr;
		NvBlastActor* blast_actor = nullptr;
		NvBlastFamily* family = nullptr;

		uint32 health = 100;
		bool is_root = true;

		friend class DestructionSystem;
	};
}