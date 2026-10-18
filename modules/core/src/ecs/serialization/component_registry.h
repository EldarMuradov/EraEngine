#pragma once

#include "core_api.h"

#include "ecs/entity.h"
#include "ecs/component.h"

#include <rttr/type>

namespace era_engine
{
    struct ComponentTypeInfo
    {
        rttr::type type;
        entt::id_type storage_id = 0;

        Component* (*add)(Entity& entity) = nullptr;
        Component* (*get)(Entity& entity) = nullptr;
        void (*remove)(Entity& entity) = nullptr;

        ERA_CORE_API bool is_serializable() const;
    };

    class ERA_CORE_API ComponentRegistry final
    {
        ComponentRegistry() = delete;

    public:
        template <typename Component_>
        static void register_component()
        {
            static_assert(std::is_base_of_v<Component, Component_>);

            ComponentTypeInfo info{rttr::type::get<Component_>()};
            info.storage_id = entt::type_hash<Component_>::value();

            if constexpr (std::is_constructible_v<Component_, ref<Entity::EcsData>&>)
            {
                info.add = [](Entity& entity) -> Component*
                {
                    return entity.add_component<Component_>();
                };
            }
            info.get = [](Entity& entity) -> Component*
            {
                return entity.get_component_if_exists<Component_>();
            };
            info.remove = [](Entity& entity)
            {
                entity.remove_component<Component_>();
            };

            register_type(info);
        }

        static const ComponentTypeInfo* find(const rttr::type& type);
        static const ComponentTypeInfo* find(std::string_view name);
        static const ComponentTypeInfo* find_by_storage_id(entt::id_type storage_id);

        static std::vector<const ComponentTypeInfo*> get_all();

    private:
        static void register_type(const ComponentTypeInfo& info);
    };
}
