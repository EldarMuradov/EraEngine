#include "ecs/prefab/prefab_utils.h"
#include "ecs/prefab/prefab_asset.h"
#include "ecs/prefab/prefab_instance_component.h"
#include "ecs/prefab/private/prefab_asset_impl.h"
#include "ecs/prefab/prefab_serializer.h"
#include "ecs/prefab/transient_component.h"

#include "ecs/serialization/component_registry.h"
#include "ecs/serialization/private/yaml_value_codec.h"

#include "ecs/base_components/transform_component.h"
#include "ecs/world.h"

#include "core/log.h"

#include <sstream>
#include <unordered_set>

namespace era_engine
{
    namespace
    {
        struct PrefabSpawnRequest
        {
            ref<PrefabAsset> prefab;
            Entity parent;
            trs transform = trs::identity;
            PrefabSpawnCallback callback;
        };

        struct PrefabSpawnQueue
        {
            std::vector<PrefabSpawnRequest> requests;
        };

        std::atomic<uint64> next_instance_id = 1;

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
            LOG_WARNING("Prefab> %s", message.c_str());
        }

        std::string get_prefab_name(const PrefabAsset& prefab)
        {
            return prefab.source_path.string();
        }

        std::vector<Entity> get_instance_entities(World* world, uint64 instance_id)
        {
            std::vector<Entity> result;
            for (auto [handle, instance_component] : world->view<PrefabInstanceComponent>().each())
            {
                if (instance_component.instance_id == instance_id)
                {
                    result.push_back(world->get_entity(handle));
                }
            }
            return result;
        }

    }

    ref<PrefabAsset> PrefabUtils::load(const fs::path& path)
    {
        ref<PrefabAsset> prefab = load_async(path);
        prefab->wait_until_loaded();
        return prefab;
    }

    ref<PrefabAsset> PrefabUtils::load_async(const fs::path& path)
    {
        GameAssetsProvider provider;
        return provider.load_game_asset_from_file<PrefabAsset>(path);
    }

    ref<PrefabAsset> PrefabUtils::create_from_string(std::string_view text)
    {
        ref<PrefabAsset> prefab = make_ref<PrefabAsset>();

        std::istringstream stream{std::string(text)};
        prefab->load_state = prefab->deserialize(stream) ? AssetLoadState::LOADED : AssetLoadState::FAILED;
        return prefab;
    }

    std::vector<Entity> PrefabUtils::instantiate(World* world, const ref<PrefabAsset>& prefab, const PrefabInstantiateParams& params)
    {
        if (world == nullptr || prefab == nullptr)
        {
            return {};
        }

        prefab->wait_until_loaded();

        if (prefab->load_state.load() != AssetLoadState::LOADED || prefab->has_failed())
        {
            LOG_ERROR("Prefab> Failed to load '%s' or its assets.", get_prefab_name(*prefab).c_str());
            return {};
        }

        const PrefabAsset::Impl& impl = *prefab->impl;

        const Entity root = world->get_root_entity();
        const Entity parent = params.parent.is_valid() ? params.parent : root;
        const uint64 instance_id = next_instance_id++;

        std::unordered_map<uint32, Entity> entities;
        std::vector<Entity> top_level_entities;

        for (const PrefabEntityDesc& desc : impl.entities)
        {
            Entity entity = world->create_entity();
            entities.emplace(desc.id, entity);

            const Entity entity_parent = desc.parent == 0 ? parent : entities.at(desc.parent);
            if (entity_parent.get_handle() != root.get_handle())
            {
                entity.set_parent(entity_parent.get_handle());
            }

            if (desc.parent == 0)
            {
                top_level_entities.push_back(entity);
            }

            PrefabInstanceComponent* instance_component = entity.add_component<PrefabInstanceComponent>();
            instance_component->prefab = prefab;
            instance_component->local_id = desc.id;
            instance_component->instance_id = instance_id;
        }

        const YamlReadContext context{&entities, &impl.dependencies};
        std::vector<Component*> deserialized_components;

        for (const PrefabEntityDesc& desc : impl.entities)
        {
            if (!desc.components || !desc.components.IsMap())
            {
                continue;
            }

            Entity entity = entities.at(desc.id);

            for (YAML::const_iterator iter = desc.components.begin(); iter != desc.components.end(); ++iter)
            {
                const std::string name = iter->first.as<std::string>();

                const ComponentTypeInfo* info = ComponentRegistry::find(name);
                if (info == nullptr || !info->is_serializable())
                {
                    warn_once(std::format("Component {} is unknown or not serializable, skipped.", name));
                    continue;
                }

                Component* component = info->add(entity);
                YamlValueCodec::read_object(iter->second, *component, context);
                deserialized_components.push_back(component);
            }
        }

        for (Component* component : deserialized_components)
        {
            component->on_deserialized();
        }

        for (Entity& entity : top_level_entities)
        {
            TransformComponent* transform = entity.get_component<TransformComponent>();
            transform->set_local_transform(params.transform * transform->get_local_transform());
        }

        return top_level_entities;
    }

    std::vector<Entity> PrefabUtils::instantiate(World* world, const fs::path& path, const PrefabInstantiateParams& params)
    {
        return instantiate(world, load_async(path), params);
    }

    void PrefabUtils::instantiate_async(World* world, const fs::path& path, const PrefabInstantiateParams& params, PrefabSpawnCallback callback)
    {
        instantiate_async(world, load_async(path), params, std::move(callback));
    }

    void PrefabUtils::instantiate_async(World* world, const ref<PrefabAsset>& prefab, const PrefabInstantiateParams& params, PrefabSpawnCallback callback)
    {
        if (world == nullptr || prefab == nullptr)
        {
            return;
        }

        PrefabSpawnRequest request;
        request.prefab = prefab;
        request.parent = params.parent;
        request.transform = params.transform;
        request.callback = std::move(callback);

        world->create_or_get_context_variable<PrefabSpawnQueue>().requests.push_back(std::move(request));
    }

    void PrefabUtils::process_spawn_queue(World* world)
    {
        if (!world->does_context_variable_exist<PrefabSpawnQueue>())
        {
            return;
        }

        std::vector<PrefabSpawnRequest> requests;
        std::swap(requests, world->get_context_variable<PrefabSpawnQueue>().requests);
        std::vector<PrefabSpawnRequest> pending_requests;

        for (PrefabSpawnRequest& request : requests)
        {
            if (request.prefab->has_failed())
            {
                LOG_ERROR("Prefab> Failed to load '%s' or its assets, spawn is cancelled.", get_prefab_name(*request.prefab).c_str());
                continue;
            }

            if (!request.prefab->is_ready())
            {
                pending_requests.push_back(std::move(request));
                continue;
            }

            PrefabInstantiateParams params;
            params.transform = request.transform;

            if (request.parent.is_valid())
            {
                params.parent = request.parent;
                if (world->get_entity(request.parent.get_handle()) != request.parent)
                {
                    LOG_WARNING("Prefab> Parent of '%s' was destroyed before spawn, spawn is cancelled.", get_prefab_name(*request.prefab).c_str());
                    continue;
                }
            }

            const std::vector<Entity> entities = instantiate(world, request.prefab, params);
            if (request.callback)
            {
                request.callback(entities);
            }
        }

        std::vector<PrefabSpawnRequest>& queued_requests = world->get_context_variable<PrefabSpawnQueue>().requests;
        queued_requests.insert(queued_requests.begin(), std::make_move_iterator(pending_requests.begin()), std::make_move_iterator(pending_requests.end()));
    }

    std::vector<Entity> PrefabUtils::get_instance_roots(const Entity& instance_entity)
    {
        if (!instance_entity.is_valid())
        {
            return {};
        }

        const PrefabInstanceComponent* instance_component = instance_entity.get_component_if_exists<PrefabInstanceComponent>();
        if (instance_component == nullptr)
        {
            return {};
        }

        World* world = instance_entity.get_world();
        const uint64 instance_id = instance_component->instance_id;

        std::vector<Entity> roots;
        for (const Entity& entity : get_instance_entities(world, instance_id))
        {
            const Entity parent = world->get_entity(entity.get_parent_handle());
            const PrefabInstanceComponent* parent_instance = parent.is_valid() ? parent.get_component_if_exists<PrefabInstanceComponent>() : nullptr;
            if (parent_instance == nullptr || parent_instance->instance_id != instance_id)
            {
                roots.push_back(entity);
            }
        }

        std::sort(roots.begin(), roots.end(), [](const Entity& a, const Entity& b)
            {
                return a.get_component<PrefabInstanceComponent>()->local_id < b.get_component<PrefabInstanceComponent>()->local_id;
            });
        return roots;
    }

    bool PrefabUtils::apply_instance(const Entity& instance_entity)
    {
        std::vector<Entity> roots = get_instance_roots(instance_entity);
        if (roots.empty())
        {
            return false;
        }

        const ref<PrefabAsset> prefab = instance_entity.get_component<PrefabInstanceComponent>()->prefab;
        if (prefab == nullptr || prefab->source_path.empty())
        {
            LOG_ERROR("Prefab> Instance has no prefab file to apply to.");
            return false;
        }

        std::vector<trs> current_transforms;
        for (Entity& root : roots)
        {
            TransformComponent* transform = root.get_component<TransformComponent>();
            current_transforms.push_back(transform->get_local_transform());
            transform->set_local_transform(prefab->get_local_transform(root.get_component<PrefabInstanceComponent>()->local_id));
        }

        const bool saved = PrefabSerializer::save_to_file(roots, prefab->source_path);

        for (size_t i = 0; i < roots.size(); ++i)
        {
            roots[i].get_component<TransformComponent>()->set_local_transform(current_transforms[i]);
        }

        return saved;
    }

    std::vector<Entity> PrefabUtils::revert_instance(const Entity& instance_entity)
    {
        const std::vector<Entity> roots = get_instance_roots(instance_entity);
        if (roots.empty())
        {
            return {};
        }

        World* world = instance_entity.get_world();
        const ref<PrefabAsset> old_prefab = instance_entity.get_component<PrefabInstanceComponent>()->prefab;
        if (old_prefab == nullptr || old_prefab->source_path.empty())
        {
            return {};
        }

        struct RootState
        {
            Entity::Handle parent = Entity::null_handle;
            trs local_transform = trs::identity;
        };

        std::unordered_map<uint32, RootState> root_states;
        for (const Entity& root : roots)
        {
            root_states[root.get_component<PrefabInstanceComponent>()->local_id] = RootState{root.get_parent_handle(), root.get_component<TransformComponent>()->get_local_transform()};
        }

        const ref<PrefabAsset> prefab = load(old_prefab->source_path);
        if (prefab->load_state.load() != AssetLoadState::LOADED || prefab->has_failed())
        {
            LOG_ERROR("Prefab> Failed to reload '%s', instance is not reverted.", get_prefab_name(*old_prefab).c_str());
            return {};
        }

        for (const Entity& root : roots)
        {
            world->destroy_entity(root.get_handle());
        }

        std::vector<Entity> new_roots = instantiate(world, prefab);
        for (Entity& root : new_roots)
        {
            const auto iter = root_states.find(root.get_component<PrefabInstanceComponent>()->local_id);
            if (iter == root_states.end())
            {
                continue;
            }

            const Entity parent = world->get_entity(iter->second.parent);
            if (parent.is_valid() && parent.get_handle() != world->get_root_entity().get_handle())
            {
                root.set_parent(parent.get_handle());
            }
            root.get_component<TransformComponent>()->set_local_transform(iter->second.local_transform);
        }

        return new_roots;
    }

    void PrefabUtils::unpack_instance(const Entity& instance_entity)
    {
        if (!instance_entity.is_valid())
        {
            return;
        }

        const PrefabInstanceComponent* instance_component = instance_entity.get_component_if_exists<PrefabInstanceComponent>();
        if (instance_component == nullptr)
        {
            return;
        }

        for (Entity& entity : get_instance_entities(instance_entity.get_world(), instance_component->instance_id))
        {
            entity.remove_component<PrefabInstanceComponent>();
        }
    }

    std::vector<Entity> PrefabUtils::duplicate(const std::vector<Entity>& entities)
    {
        const auto is_descendant_of_any = [&entities](const Entity& entity)
        {
            World* world = entity.get_world();
            for (Entity::Handle ancestor = entity.get_parent_handle(); ancestor != Entity::null_handle; ancestor = world->get_entity(ancestor).get_parent_handle())
            {
                for (const Entity& other : entities)
                {
                    if (other.is_valid() && other.get_handle() == ancestor && other.get_world() == world)
                    {
                        return true;
                    }
                }
                if (!world->get_entity(ancestor).is_valid())
                {
                    break;
                }
            }
            return false;
        };

        // Same filtering as PrefabSerializer, so copies match sources by index.
        std::vector<Entity> sources;
        for (const Entity& entity : entities)
        {
            if (entity.is_valid() && entity != entity.get_world()->get_root_entity() && !entity.has_component<TransientComponent>() &&
                !is_descendant_of_any(entity) && std::find(sources.begin(), sources.end(), entity) == sources.end())
            {
                sources.push_back(entity);
            }
        }

        if (sources.empty())
        {
            return {};
        }

        World* world = sources.front().get_world();
        const ref<PrefabAsset> prefab = create_from_string(PrefabSerializer::save_to_string(sources));

        std::vector<Entity> copies = instantiate(world, prefab);
        if (copies.empty())
        {
            return {};
        }

        unpack_instance(copies.front());

        for (size_t i = 0; i < copies.size() && i < sources.size(); ++i)
        {
            const Entity::Handle parent_handle = sources[i].get_parent_handle();
            if (parent_handle != world->get_root_entity().get_handle())
            {
                copies[i].set_parent(parent_handle);
            }
        }
        return copies;
    }

    size_t PrefabUtils::get_pending_spawn_count(World* world)
    {
        if (!world->does_context_variable_exist<PrefabSpawnQueue>())
        {
            return 0;
        }
        return world->get_context_variable<PrefabSpawnQueue>().requests.size();
    }
}
