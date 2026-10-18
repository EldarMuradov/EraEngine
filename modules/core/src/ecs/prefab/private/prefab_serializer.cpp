#include "ecs/prefab/prefab_serializer.h"
#include "ecs/prefab/prefab_asset.h"
#include "ecs/prefab/transient_component.h"

#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/private/yaml_value_codec.h"

#include "ecs/world.h"

#include "core/log.h"

#include <fstream>
#include <unordered_set>

namespace era_engine
{
    namespace
    {
        struct SavedEntity
        {
            Entity entity;
            uint32 id = 0;
            uint32 parent = 0;
        };

        struct SaveState
        {
            std::vector<SavedEntity> entities;
            std::unordered_map<Entity::Handle, uint32> ids;
        };

        void collect_entity(const Entity& entity, uint32 parent_id, SaveState& state)
        {
            if (!entity.is_valid() || state.ids.contains(entity.get_handle()) || entity.has_component<TransientComponent>())
            {
                return;
            }

            const uint32 id = static_cast<uint32>(state.entities.size()) + 1;
            state.ids.emplace(entity.get_handle(), id);
            state.entities.push_back(SavedEntity{entity, id, parent_id});

            for (const Entity& child : EntityContainer::get_entity_childs(entity.get_world(), entity.get_handle()))
            {
                collect_entity(child, id, state);
            }
        }

        YAML::Node write_components(Entity entity, const YamlWriteContext& context)
        {
            std::vector<const ComponentTypeInfo*> component_types;

            entt::registry& registry = entity.get_world()->get_registry();
            for (auto [storage_id, storage] : registry.storage())
            {
                if (!storage.contains(entity.get_handle()))
                {
                    continue;
                }

                const ComponentTypeInfo* info = ComponentRegistry::find_by_storage_id(storage_id);
                if (info == nullptr)
                {
                    static std::mutex sync;
                    static std::unordered_set<entt::id_type> reported;
                    std::lock_guard _lock{sync};
                    if (reported.insert(storage_id).second)
                    {
                        const std::string name(storage.type().name());
                        LOG_WARNING("Prefab> Component %s is not registered in ComponentRegistry and is not saved.", name.c_str());
                    }
                    continue;
                }

                if (info->is_serializable())
                {
                    component_types.push_back(info);
                }
            }

            std::sort(component_types.begin(), component_types.end(), [](const ComponentTypeInfo* a, const ComponentTypeInfo* b)
                {
                    return a->type.get_name() < b->type.get_name();
                });

            YAML::Node components(YAML::NodeType::Map);
            for (const ComponentTypeInfo* info : component_types)
            {
                Component* component = info->get(entity);
                components[info->type.get_name().to_string()] = YamlValueCodec::write_object(*component, context);
            }
            return components;
        }
    }

    std::string PrefabSerializer::save_to_string(const std::vector<Entity>& top_level_entities)
    {
        SaveState state;
        for (const Entity& entity : top_level_entities)
        {
            if (!entity.is_valid())
            {
                continue;
            }

            World* world = entity.get_world();
            if (entity == world->get_root_entity())
            {
                for (const Entity& child : EntityContainer::get_entity_childs(world, entity.get_handle()))
                {
                    collect_entity(child, 0, state);
                }
            }
            else
            {
                collect_entity(entity, 0, state);
            }
        }

        const YamlWriteContext context{&state.ids};

        YAML::Node entities(YAML::NodeType::Sequence);
        for (const SavedEntity& saved_entity : state.entities)
        {
            YAML::Node entity_node(YAML::NodeType::Map);
            entity_node["id"] = saved_entity.id;
            entity_node["parent"] = saved_entity.parent;
            entity_node["components"] = write_components(saved_entity.entity, context);
            entities.push_back(entity_node);
        }

        YAML::Node document(YAML::NodeType::Map);
        document["version"] = 1;
        document["entities"] = entities;

        YAML::Emitter emitter;
        emitter << document;
        return emitter.c_str();
    }

    bool PrefabSerializer::save_to_file(const std::vector<Entity>& top_level_entities, const fs::path& path)
    {
        const std::string text = save_to_string(top_level_entities);

        const fs::path final_path = fs::absolute(path).lexically_normal().make_preferred();
        std::error_code error;
        fs::create_directories(final_path.parent_path(), error);

        std::ofstream os(final_path, std::ios_base::binary | std::ios_base::trunc);
        if (!os.is_open())
        {
            LOG_ERROR("Prefab> Failed to open '%s' for writing.", final_path.string().c_str());
            return false;
        }

        os << text;
        os.close();

        GameAssetCache::remove_asset_from_cache(final_path, 0);
        return !os.fail();
    }
}
