#include "ecs/serialization/private/yaml_value_codec.h"
#include "ecs/serialization/serialization_meta.h"
#include "ecs/serialization/value_adapters.h"

#include "core/yaml.h"
#include "core/log.h"

#include <unordered_set>

namespace era_engine
{
    namespace
    {
        struct LeafCodec
        {
            YAML::Node (*write)(const rttr::variant& value) = nullptr;
            void (*read)(const YAML::Node& node, rttr::variant& value) = nullptr;
        };

        template <typename Type_>
        LeafCodec make_leaf_codec()
        {
            return LeafCodec{
                [](const rttr::variant& value)
                {
                    return YAML::Node(value.get_value<Type_>());
                },
                [](const YAML::Node& node, rttr::variant& value)
                {
                    value = node.as<Type_>();
                }};
        }

        template <typename Type_>
        LeafCodec make_byte_codec()
        {
            return LeafCodec{
                [](const rttr::variant& value)
                {
                    return YAML::Node(static_cast<int32>(value.get_value<Type_>()));
                },
                [](const YAML::Node& node, rttr::variant& value)
                {
                    value = static_cast<Type_>(node.as<int32>());
                }};
        }

        template <typename Type_>
        void add_leaf_codec(std::unordered_map<rttr::type::type_id, LeafCodec>& codecs, LeafCodec codec)
        {
            codecs.emplace(rttr::type::get<Type_>().get_id(), codec);
        }

        const LeafCodec* find_leaf_codec(const rttr::type& type)
        {
            static const std::unordered_map<rttr::type::type_id, LeafCodec> codecs = []()
            {
                std::unordered_map<rttr::type::type_id, LeafCodec> result;
                add_leaf_codec<bool>(result, make_leaf_codec<bool>());
                add_leaf_codec<int8>(result, make_byte_codec<int8>());
                add_leaf_codec<uint8>(result, make_byte_codec<uint8>());
                add_leaf_codec<int16>(result, make_leaf_codec<int16>());
                add_leaf_codec<uint16>(result, make_leaf_codec<uint16>());
                add_leaf_codec<int32>(result, make_leaf_codec<int32>());
                add_leaf_codec<uint32>(result, make_leaf_codec<uint32>());
                add_leaf_codec<int64>(result, make_leaf_codec<int64>());
                add_leaf_codec<uint64>(result, make_leaf_codec<uint64>());
                add_leaf_codec<float>(result, make_leaf_codec<float>());
                add_leaf_codec<double>(result, make_leaf_codec<double>());
                add_leaf_codec<std::string>(result, make_leaf_codec<std::string>());
                add_leaf_codec<vec2>(result, make_leaf_codec<vec2>());
                add_leaf_codec<vec3>(result, make_leaf_codec<vec3>());
                add_leaf_codec<vec4>(result, make_leaf_codec<vec4>());
                add_leaf_codec<quat>(result, make_leaf_codec<quat>());
                add_leaf_codec<mat3>(result, make_leaf_codec<mat3>());
                add_leaf_codec<mat4>(result, make_leaf_codec<mat4>());
                add_leaf_codec<trs>(result, make_leaf_codec<trs>());
                add_leaf_codec<fs::path>(result, make_leaf_codec<fs::path>());
                return result;
            }();

            const auto iter = codecs.find(type.get_id());
            return iter != codecs.end() ? &iter->second : nullptr;
        }

        void warn_once(const std::string& message)
        {
            static std::mutex sync;
            static std::unordered_set<std::string> reported;
            {
                std::lock_guard _lock{sync};
                if (!reported.insert(message).second)
                {
                    return;
                }
            }
            LOG_WARNING("Serialization> %s", message.c_str());
        }

        uint32 get_entity_id(Entity::Handle handle, const YamlWriteContext& context)
        {
            if (handle == Entity::null_handle || context.entity_ids == nullptr)
            {
                return 0;
            }

            const auto iter = context.entity_ids->find(handle);
            if (iter == context.entity_ids->end())
            {
                warn_once("Reference to an entity outside of the serialized set is written as null.");
                return 0;
            }
            return iter->second;
        }

        Entity find_entity(const YAML::Node& node, const YamlReadContext& context)
        {
            const uint32 id = node.as<uint32>();
            if (id == 0 || context.entities == nullptr)
            {
                return Entity::null;
            }

            const auto iter = context.entities->find(id);
            if (iter == context.entities->end())
            {
                warn_once(std::format("Unknown entity id {} is read as null.", id));
                return Entity::null;
            }
            return iter->second;
        }

        YAML::Node make_null_node()
        {
            return YAML::Node(YAML::NodeType::Null);
        }
    }

    YAML::Node YamlValueCodec::write_object(const rttr::instance& object, const YamlWriteContext& context)
    {
        YAML::Node result(YAML::NodeType::Map);

        const rttr::type type = object.get_derived_type();
        for (const rttr::property& property : type.get_properties())
        {
            if (property.is_readonly() || property.is_static() || !is_serializable(property))
            {
                continue;
            }

            const rttr::variant value = property.get_value(object);
            std::optional<YAML::Node> node = write_value(value, context);
            if (!node.has_value())
            {
                warn_once(std::format("Property {}::{} of type {} is not serializable.",
                    type.get_name().to_string(), property.get_name().to_string(), property.get_type().get_name().to_string()));
                continue;
            }

            result[property.get_name().to_string()] = *node;
        }

        return result;
    }

    void YamlValueCodec::read_object(const YAML::Node& node, const rttr::instance& object, const YamlReadContext& context)
    {
        const rttr::type type = object.get_derived_type();

        if (!node.IsMap())
        {
            if (!node.IsNull())
            {
                warn_once(std::format("Expected a map for {}.", type.get_name().to_string()));
            }
            return;
        }

        for (YAML::const_iterator iter = node.begin(); iter != node.end(); ++iter)
        {
            const std::string name = iter->first.as<std::string>();
            const rttr::property property = type.get_property(name);
            if (!property.is_valid())
            {
                warn_once(std::format("Unknown property {}::{} is skipped.", type.get_name().to_string(), name));
                continue;
            }

            if (property.is_readonly() || property.is_static() || !is_serializable(property))
            {
                continue;
            }

            rttr::variant value = property.get_value(object);

            bool read = false;
            try
            {
                read = read_value(iter->second, value, context);
            }
            catch (const YAML::Exception& exception)
            {
                warn_once(std::format("Failed to read {}::{}: {}", type.get_name().to_string(), name, exception.what()));
                continue;
            }

            if (!read || !property.set_value(object, value))
            {
                warn_once(std::format("Failed to read {}::{}.", type.get_name().to_string(), name));
            }
        }
    }

    std::optional<YAML::Node> YamlValueCodec::write_value(const rttr::variant& value, const YamlWriteContext& context)
    {
        const rttr::type type = value.get_type();

        if (const LeafCodec* codec = find_leaf_codec(type))
        {
            return codec->write(value);
        }

        if (type == rttr::type::get<Entity::Handle>())
        {
            return YAML::Node(get_entity_id(value.get_value<Entity::Handle>(), context));
        }

        if (type == rttr::type::get<EntityPtr>())
        {
            const Entity entity = value.get_value<EntityPtr>().get();
            return YAML::Node(entity.is_valid() ? get_entity_id(entity.get_handle(), context) : 0u);
        }

        if (const ValueAdapter* adapter = ValueAdapters::find(type))
        {
            const rttr::variant inner = adapter->unwrap(value);
            if (!inner.is_valid())
            {
                return make_null_node();
            }
            return write_value(inner, context);
        }

        if (const AssetTypeInfo* asset_type = AssetTypeRegistry::find(type))
        {
            const ref<GameAsset> asset = asset_type->from_variant(value);
            if (asset == nullptr)
            {
                return make_null_node();
            }

            const std::optional<std::string> path = AssetTypeRegistry::to_relative_path(asset->source_path);
            if (!path.has_value())
            {
                LOG_ERROR("Serialization> Asset '%s' is not inside the assets root and is written as null.", asset->source_path.string().c_str());
                return make_null_node();
            }

            YAML::Node node(YAML::NodeType::Map);
            node["asset"] = *path;
            if (asset->flags != 0)
            {
                node["flags"] = asset->flags;
            }
            return node;
        }

        if (type.is_enumeration())
        {
            const rttr::enumeration enumeration = type.get_enumeration();
            if (!enumeration.is_valid())
            {
                return std::nullopt;
            }

            const rttr::string_view name = enumeration.value_to_name(value);
            if (!name.empty())
            {
                return YAML::Node(name.to_string());
            }

            if (const EnumConverter* converter = ValueAdapters::find_enum_converter(type))
            {
                return YAML::Node(converter->to_integer(value));
            }

            bool converted = false;
            const int64 raw_value = value.to_int64(&converted);
            return converted ? std::optional<YAML::Node>(YAML::Node(raw_value)) : std::nullopt;
        }

        if (value.is_sequential_container())
        {
            const rttr::variant_sequential_view view = value.create_sequential_view();

            YAML::Node node(YAML::NodeType::Sequence);
            for (size_t i = 0; i < view.get_size(); ++i)
            {
                std::optional<YAML::Node> item = write_value(view.get_value(i).extract_wrapped_value(), context);
                if (!item.has_value())
                {
                    return std::nullopt;
                }
                node.push_back(*item);
            }
            return node;
        }

        if (type.is_class() && !type.is_wrapper() && !type.get_properties().empty())
        {
            return write_object(rttr::instance(value), context);
        }

        return std::nullopt;
    }

    bool YamlValueCodec::read_value(const YAML::Node& node, rttr::variant& value, const YamlReadContext& context)
    {
        const rttr::type type = value.get_type();

        if (const LeafCodec* codec = find_leaf_codec(type))
        {
            codec->read(node, value);
            return true;
        }

        if (type == rttr::type::get<Entity::Handle>())
        {
            const Entity entity = find_entity(node, context);
            value = entity.is_valid() ? entity.get_handle() : Entity::Handle(Entity::null_handle);
            return true;
        }

        if (type == rttr::type::get<EntityPtr>())
        {
            const Entity entity = find_entity(node, context);
            value = entity.is_valid() ? EntityPtr(entity) : EntityPtr();
            return true;
        }

        if (const ValueAdapter* adapter = ValueAdapters::find(type))
        {
            if (node.IsNull())
            {
                rttr::variant wrapped = adapter->wrap(rttr::variant());
                if (!wrapped.is_valid())
                {
                    return false;
                }
                value = std::move(wrapped);
                return true;
            }

            rttr::variant inner = adapter->unwrap(value);
            if (!inner.is_valid())
            {
                inner = adapter->make_default_inner();
            }

            if (!read_value(node, inner, context))
            {
                return false;
            }

            value = adapter->wrap(inner);
            return value.is_valid();
        }

        if (const AssetTypeInfo* asset_type = AssetTypeRegistry::find(type))
        {
            if (node.IsNull())
            {
                value = asset_type->to_variant(nullptr);
                return true;
            }

            const std::optional<AssetReference> reference = parse_asset_reference(node);
            if (!reference.has_value())
            {
                return false;
            }

            ref<GameAsset> asset = nullptr;
            if (context.assets != nullptr)
            {
                const auto iter = context.assets->find(*reference);
                if (iter != context.assets->end())
                {
                    asset = iter->second;
                }
            }

            if (asset == nullptr)
            {
                asset = asset_type->load(AssetTypeRegistry::to_absolute_path(reference->path), reference->flags);
            }

            rttr::variant typed_asset = asset_type->to_variant(asset);
            if (asset_type->from_variant(typed_asset) == nullptr)
            {
                warn_once(std::format("Asset '{}' is not of type {}.", reference->path, type.get_name().to_string()));
                return false;
            }

            value = std::move(typed_asset);
            return true;
        }

        if (type.is_enumeration())
        {
            const rttr::enumeration enumeration = type.get_enumeration();
            if (!enumeration.is_valid() || !node.IsScalar())
            {
                return false;
            }

            const std::string name = node.as<std::string>();
            rttr::variant enum_value = enumeration.name_to_value(name);
            if (enum_value.is_valid())
            {
                value = std::move(enum_value);
                return true;
            }

            int64 raw_value = 0;
            if (!YAML::convert<int64>::decode(node, raw_value))
            {
                return false;
            }

            if (const EnumConverter* converter = ValueAdapters::find_enum_converter(type))
            {
                value = converter->from_integer(raw_value);
                return true;
            }

            for (const rttr::variant& candidate : enumeration.get_values())
            {
                if (candidate.to_int64() == raw_value)
                {
                    value = candidate;
                    return true;
                }
            }
            return false;
        }

        if (value.is_sequential_container())
        {
            if (!node.IsSequence())
            {
                return false;
            }

            rttr::variant_sequential_view view = value.create_sequential_view();
            if (view.is_dynamic())
            {
                if (!view.set_size(node.size()))
                {
                    return false;
                }
            }
            else if (view.get_size() != node.size())
            {
                warn_once(std::format("Fixed size container {} has {} elements, data has {}.", type.get_name().to_string(), view.get_size(), node.size()));
            }

            const size_t count = std::min(view.get_size(), node.size());
            for (size_t i = 0; i < count; ++i)
            {
                rttr::variant item = view.get_value(i).extract_wrapped_value();
                if (!read_value(node[i], item, context) || !view.set_value(i, item))
                {
                    return false;
                }
            }
            return true;
        }

        if (type.is_class() && !type.is_wrapper() && !type.get_properties().empty())
        {
            read_object(node, rttr::instance(value), context);
            return true;
        }

        return false;
    }

    std::optional<AssetReference> YamlValueCodec::parse_asset_reference(const YAML::Node& node)
    {
        if (!node.IsMap())
        {
            return std::nullopt;
        }

        const YAML::Node path_node = node["asset"];
        if (!path_node || !path_node.IsScalar())
        {
            return std::nullopt;
        }

        AssetReference reference{path_node.as<std::string>()};
        size_t expected_size = 1;

        const YAML::Node flags_node = node["flags"];
        if (flags_node)
        {
            if (!flags_node.IsScalar() || !YAML::convert<uint32>::decode(flags_node, reference.flags))
            {
                return std::nullopt;
            }
            ++expected_size;
        }

        if (node.size() != expected_size)
        {
            return std::nullopt;
        }
        return reference;
    }

    void YamlValueCodec::collect_asset_references(const YAML::Node& node, std::vector<AssetReference>& out)
    {
        if (std::optional<AssetReference> reference = parse_asset_reference(node))
        {
            out.push_back(std::move(*reference));
            return;
        }

        if (node.IsMap())
        {
            for (YAML::const_iterator iter = node.begin(); iter != node.end(); ++iter)
            {
                collect_asset_references(iter->second, out);
            }
        }
        else if (node.IsSequence())
        {
            for (YAML::const_iterator iter = node.begin(); iter != node.end(); ++iter)
            {
                collect_asset_references(*iter, out);
            }
        }
    }
}
