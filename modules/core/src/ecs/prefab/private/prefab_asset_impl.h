#pragma once

#include "ecs/prefab/prefab_asset.h"

#include "asset/asset_type_registry.h"

#include <yaml-cpp/yaml.h>

namespace era_engine
{
	struct PrefabEntityDesc
	{
		uint32 id = 0;
		uint32 parent = 0;
		YAML::Node components;
	};

	struct PrefabAsset::Impl
	{
		YAML::Node document;
		std::vector<PrefabEntityDesc> entities;
		std::unordered_map<AssetReference, ref<GameAsset>, AssetReferenceHash> dependencies;
	};
}
