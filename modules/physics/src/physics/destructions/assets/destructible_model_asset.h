#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"
#include "physics/destructions/destruction_types.h"

namespace era_engine::physics
{
	class ERA_PHYSICS_API DestructibleModelAsset : public DestructibleAsset
	{
	public:

		DestructibleModelAsset();
		~DestructibleModelAsset() override;

		ref<NvMesh> model = nullptr;
	};
}