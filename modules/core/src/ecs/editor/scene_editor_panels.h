#pragma once

#include "core_api.h"

#include "core/math.h"

#include "ecs/entity.h"

#include <functional>

namespace era_engine
{
    class World;

    class ERA_CORE_API SceneEditorPanels final
    {
    public:
        SceneEditorPanels(World* _world);

        void draw();

        Entity get_selected_entity() const;
        void set_selected_entity(const Entity& entity);

        void create_empty(const Entity& parent = Entity());
        void create_point_light();
        void create_spot_light();

        void save_as_prefab_with_dialog(const Entity& entity);
        void instantiate_prefab_with_dialog(const Entity& parent = Entity());
        void instantiate_prefab(const fs::path& path, const Entity& parent = Entity());

    private:
        void draw_hierarchy();
        void draw_entity_node(const Entity& entity);
        void draw_entity_context_menu(const Entity& entity);
        void draw_hierarchy_drop_target(const Entity& parent);

        void draw_inspector();
        void draw_prefab_instance_section(const Entity& entity);

        void reparent(Entity entity, const Entity& new_parent);
        void destroy(const Entity& entity);

        trs get_spawn_transform() const;

        void defer(std::function<void()> action);

    private:
        World* world = nullptr;

        Entity selected_entity;

        std::vector<std::function<void()>> deferred_actions;
    };
}
