#include "physics/destructions/destruction_types.h"

namespace era_engine::physics
{
	DestructibleAsset::DestructibleAsset()
	{
	}

	DestructibleAsset::~DestructibleAsset()
	{
		if (blast_asset)
		{
			free(blast_asset);
			blast_asset = nullptr;
		}
	}

}