#pragma once

#include "ecs/entity.h"

#include "asset/asset_type_registry.h"

#include <yaml-cpp/yaml.h>

#include <rttr/type>

namespace era_engine
{
	struct YamlWriteContext
	{
		// Entity references outside this map are written as 0. Without the map every entity reference is written as 0.
		const std::unordered_map<Entity::Handle, uint32>* entity_ids = nullptr;
	};

	struct YamlReadContext
	{
		const std::unordered_map<uint32, Entity>* entities = nullptr;
		// Preloaded assets. Missing references are loaded asynchronously on demand.
		const std::unordered_map<AssetReference, ref<GameAsset>, AssetReferenceHash>* assets = nullptr;
	};

	// Converts reflected values to YAML and back. YAML objects never leave core (yaml-cpp is linked statically into each DLL).
	class YamlValueCodec final
	{
		YamlValueCodec() = delete;

	public:
		static YAML::Node write_object(const rttr::instance& object, const YamlWriteContext& context);
		static void read_object(const YAML::Node& node, const rttr::instance& object, const YamlReadContext& context);

		// Returns nullopt if the value type is not serializable.
		static std::optional<YAML::Node> write_value(const rttr::variant& value, const YamlWriteContext& context);
		// `value` holds the current value of the target type and is replaced with the read one.
		static bool read_value(const YAML::Node& node, rttr::variant& value, const YamlReadContext& context);

		static std::optional<AssetReference> parse_asset_reference(const YAML::Node& node);
		static void collect_asset_references(const YAML::Node& node, std::vector<AssetReference>& out);
	};
}
