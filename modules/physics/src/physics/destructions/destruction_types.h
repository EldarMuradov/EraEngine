#pragma once

#include "physics_api.h"

#include <NvBlast.h>

namespace era_engine::physics
{
	class ERA_PHYSICS_API DestructibleAsset
	{
	public:
		DestructibleAsset();
		virtual ~DestructibleAsset();

	protected:
		NvBlastAsset* blast_asset = nullptr;

		std::vector<NvBlastChunkDesc> chunks;
		std::vector<NvBlastBondDesc> bonds;

		friend class DestructionUtils;
		friend class DestructionSystem;
	};
}