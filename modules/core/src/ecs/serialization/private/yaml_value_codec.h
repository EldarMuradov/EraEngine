#pragma once

#include "ecs/entity.h"

#include "asset/asset_type_registry.h"

#include <yaml-cpp/yaml.h>

#include <rttr/type>

namespace era_engine
{
    struct YamlWriteContext
    {
        const std::unordered_map<Entity::Handle, uint32>* entity_ids = nullptr;
    };

    struct YamlReadContext
    {
        const std::unordered_map<uint32, Entity>* entities = nullptr;
        const std::unordered_map<AssetReference, ref<GameAsset>, AssetReferenceHash>* assets = nullptr;
    };

    class YamlValueCodec final
    {
        YamlValueCodec() = delete;

    public:
        static YAML::Node write_object(const rttr::instance& object, const YamlWriteContext& context);
        static void read_object(const YAML::Node& node, const rttr::instance& object, const YamlReadContext& context);

        static std::optional<YAML::Node> write_value(const rttr::variant& value, const YamlWriteContext& context);
        static bool read_value(const YAML::Node& node, rttr::variant& value, const YamlReadContext& context);

        static std::optional<AssetReference> parse_asset_reference(const YAML::Node& node);
        static void collect_asset_references(const YAML::Node& node, std::vector<AssetReference>& out);
    };
}
