#include "ecs/editor/scene_editor_panels.h"
#include "ecs/editor/entity_editor_utils.h"

#include "ecs/world.h"
#include "ecs/base_components/base_components.h"
#include "ecs/prefab/prefab_asset.h"
#include "ecs/prefab/prefab_instance_component.h"
#include "ecs/prefab/prefab_serializer.h"
#include "ecs/prefab/prefab_utils.h"
#include "ecs/prefab/transient_component.h"

#include "asset/asset_type_registry.h"

#include "rendering/light_source.h"
#include "rendering/ecs/renderer_holder_root_component.h"

#include "editor/file_dialog.h"

#include "core/editor_icons.h"
#include "core/imgui.h"
#include "core/input.h"
#include "core/log.h"

#include <imgui/imgui_internal.h>

namespace era_engine
{
    namespace
    {
        const ImVec4 PREFAB_INSTANCE_COLOR = ImVec4(0.45f, 0.75f, 1.0f, 1.0f);

        std::string get_entity_name(const Entity& entity)
        {
            const NameComponent* name_component = entity.get_component_if_exists<NameComponent>();
            if (name_component != nullptr && name_component->name[0] != 0)
            {
                return name_component->name;
            }
            return std::format("Entity {}", static_cast<uint32>(entity.get_handle()));
        }

        bool is_prefab_instance_root(World* world, const Entity& entity)
        {
            const PrefabInstanceComponent* instance_component = entity.get_component_if_exists<PrefabInstanceComponent>();
            if (instance_component == nullptr)
            {
                return false;
            }

            const Entity parent = world->get_entity(entity.get_parent_handle());
            const PrefabInstanceComponent* parent_instance = parent.is_valid() ? parent.get_component_if_exists<PrefabInstanceComponent>() : nullptr;
            return parent_instance == nullptr || parent_instance->instance_id != instance_component->instance_id;
        }

        bool is_alive(World* world, const Entity& entity)
        {
            return entity.is_valid() && world->get_entity(entity.get_handle()) == entity;
        }

        bool is_same_or_ancestor(World* world, const Entity& ancestor, const Entity& entity)
        {
            Entity current = entity;
            while (current.is_valid())
            {
                if (current == ancestor)
                {
                    return true;
                }
                current = world->get_entity(current.get_parent_handle());
            }
            return false;
        }

        bool disabled_button(const char* label, bool enabled)
        {
            if (!enabled)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            const bool pressed = ImGui::Button(label);

            if (!enabled)
            {
                ImGui::PopStyleVar();
                ImGui::PopItemFlag();
            }
            return pressed && enabled;
        }
    }

    SceneEditorPanels::SceneEditorPanels(World* _world)
        : world(_world)
    {
    }

    void SceneEditorPanels::draw()
    {
        if (selected_entity.is_valid() && !is_alive(world, selected_entity))
        {
            selected_entity = Entity();
        }

        draw_hierarchy();
        draw_inspector();

        std::vector<std::function<void()>> actions;
        std::swap(actions, deferred_actions);
        for (const std::function<void()>& action : actions)
        {
            action();
        }
    }

    Entity SceneEditorPanels::get_selected_entity() const
    {
        return selected_entity;
    }

    void SceneEditorPanels::set_selected_entity(const Entity& entity)
    {
        selected_entity = entity;
    }

    void SceneEditorPanels::create_empty(const Entity& parent)
    {
        Entity entity = world->create_entity("Empty");
        if (parent.is_valid() && parent != world->get_root_entity())
        {
            entity.set_parent(parent.get_handle());
        }
        else
        {
            entity.get_component<TransformComponent>()->set_local_transform(get_spawn_transform());
        }
        set_selected_entity(entity);
    }

    void SceneEditorPanels::create_point_light()
    {
        Entity entity = world->create_entity("Point light");
        entity.add_component<PointLightComponent>(vec3(1.0f), 1.0f, 10.0f, true, 2048u);
        entity.get_component<TransformComponent>()->set_local_transform(get_spawn_transform());
        set_selected_entity(entity);
    }

    void SceneEditorPanels::create_spot_light()
    {
        Entity entity = world->create_entity("Spot light");
        entity.add_component<SpotLightComponent>(vec3(1.0f), 1.0f, 25.0f, deg2rad(20.0f), deg2rad(30.0f), true, 2048u);
        entity.get_component<TransformComponent>()->set_local_transform(get_spawn_transform());
        set_selected_entity(entity);
    }

    void SceneEditorPanels::save_as_prefab_with_dialog(const Entity& entity)
    {
        if (!is_alive(world, entity))
        {
            return;
        }

        const std::string path = save_file_dialog("Era prefab", "eprefab");
        if (path.empty())
        {
            return;
        }

        if (PrefabSerializer::save_to_file({entity}, path))
        {
            LOG_MESSAGE("Editor> Saved prefab '%s'.", path.c_str());
        }
    }

    void SceneEditorPanels::instantiate_prefab_with_dialog(const Entity& parent)
    {
        const std::string path = open_file_dialog("Era prefab", "eprefab");
        if (!path.empty())
        {
            instantiate_prefab(path, parent);
        }
    }

    void SceneEditorPanels::instantiate_prefab(const fs::path& path, const Entity& parent)
    {
        PrefabInstantiateParams params;
        if (parent.is_valid() && parent != world->get_root_entity())
        {
            params.parent = parent;
        }
        else
        {
            params.transform = get_spawn_transform();
        }

        PrefabUtils::instantiate_async(world, path, params, [this](const std::vector<Entity>& entities)
            {
                if (!entities.empty())
                {
                    set_selected_entity(entities.front());
                }
            });
    }

    void SceneEditorPanels::draw_hierarchy()
    {
        if (ImGui::Begin("Scene Hierarchy"))
        {
            const Entity root = world->get_root_entity();
            for (const Entity& child : EntityContainer::get_entity_childs(world, root.get_handle()))
            {
                draw_entity_node(child);
            }

            ImVec2 free_space = ImGui::GetContentRegionAvail();
            free_space.x = max(free_space.x, 1.0f);
            free_space.y = max(free_space.y, ImGui::GetFrameHeight());
            ImGui::InvisibleButton("##hierarchy_background", free_space);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                set_selected_entity(Entity());
            }

            draw_hierarchy_drop_target(root);

            if (ImGui::BeginPopupContextItem("HierarchyContext"))
            {
                if (ImGui::MenuItem("Create empty"))
                {
                    defer([this]()
                        {
                            create_empty();
                        });
                }
                if (ImGui::MenuItem("Create point light"))
                {
                    defer([this]()
                        {
                            create_point_light();
                        });
                }
                if (ImGui::MenuItem("Create spot light"))
                {
                    defer([this]()
                        {
                            create_spot_light();
                        });
                }
                ImGui::Separator();
                if (ImGui::MenuItem(EDITOR_ICON_PREFAB "  Instantiate prefab..."))
                {
                    defer([this]()
                        {
                            instantiate_prefab_with_dialog();
                        });
                }
                ImGui::EndPopup();
            }

            if (selected_entity.is_valid() && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive())
            {
                const Entity entity = selected_entity;
                if (ImGui::IsKeyPressed(KEY_DELETE))
                {
                    defer([this, entity]()
                        {
                            destroy(entity);
                        });
                }
                else if (ImGui::IsKeyDown(KEY_CTRL) && ImGui::IsKeyPressed('D'))
                {
                    defer([this, entity]()
                        {
                            const std::vector<Entity> copies = PrefabUtils::duplicate({entity});
                            if (!copies.empty())
                            {
                                set_selected_entity(copies.front());
                            }
                        });
                }
            }
        }
        ImGui::End();
    }

    void SceneEditorPanels::draw_entity_node(const Entity& entity)
    {
        const Entity::Handle handle = entity.get_handle();
        const std::vector<Entity> childs = EntityContainer::get_entity_childs(world, handle);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (childs.empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }
        if (selected_entity == entity)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool is_instance = entity.has_component<PrefabInstanceComponent>();
        std::string label = get_entity_name(entity);
        if (is_instance && is_prefab_instance_root(world, entity))
        {
            label = std::string(EDITOR_ICON_PREFAB "  ") + label;
        }

        const bool is_transient = entity.has_component<TransientComponent>();
        if (is_transient || is_instance)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, is_transient ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled) : PREFAB_INSTANCE_COLOR);
        }
        const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint32>(handle))), flags, "%s", label.c_str());
        if (is_transient || is_instance)
        {
            ImGui::PopStyleColor();
        }

        if (is_transient && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Created at runtime, not saved");
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen())
        {
            set_selected_entity(entity);
        }

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(EDITOR_PAYLOAD_ENTITY, &handle, sizeof(handle));
            ImGui::TextUnformatted(label.c_str());
            ImGui::EndDragDropSource();
        }

        draw_hierarchy_drop_target(entity);
        draw_entity_context_menu(entity);

        if (open)
        {
            for (const Entity& child : childs)
            {
                draw_entity_node(child);
            }
            ImGui::TreePop();
        }
    }

    void SceneEditorPanels::draw_entity_context_menu(const Entity& entity)
    {
        if (!ImGui::BeginPopupContextItem())
        {
            return;
        }

        const Entity root = world->get_root_entity();

        if (ImGui::MenuItem("Create child"))
        {
            defer([this, entity]()
                {
                    create_empty(entity);
                });
        }
        if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
        {
            defer([this, entity]()
                {
                    const std::vector<Entity> copies = PrefabUtils::duplicate({entity});
                    if (!copies.empty())
                    {
                        set_selected_entity(copies.front());
                    }
                });
        }
        if (ImGui::MenuItem("Move to root", nullptr, false, entity.get_parent_handle() != root.get_handle()))
        {
            defer([this, entity, root]()
                {
                    reparent(entity, root);
                });
        }

        ImGui::Separator();

        if (ImGui::MenuItem(EDITOR_ICON_PREFAB "  Save as prefab..."))
        {
            defer([this, entity]()
                {
                    save_as_prefab_with_dialog(entity);
                });
        }
        if (ImGui::MenuItem("Instantiate prefab as child..."))
        {
            defer([this, entity]()
                {
                    instantiate_prefab_with_dialog(entity);
                });
        }

        ImGui::Separator();

        if (ImGui::MenuItem(ICON_FA_TRASH_ALT "  Delete", "Del"))
        {
            defer([this, entity]()
                {
                    destroy(entity);
                });
        }

        ImGui::EndPopup();
    }

    void SceneEditorPanels::draw_hierarchy_drop_target(const Entity& parent)
    {
        if (!ImGui::BeginDragDropTarget())
        {
            return;
        }

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EDITOR_PAYLOAD_ENTITY))
        {
            const Entity dragged = world->get_entity(*static_cast<const Entity::Handle*>(payload->Data));
            defer([this, dragged, parent]()
                {
                    reparent(dragged, parent);
                });
        }

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EDITOR_ICON_PREFAB))
        {
            const fs::path path = static_cast<const char*>(payload->Data);
            defer([this, path, parent]()
                {
                    instantiate_prefab(path, parent);
                });
        }

        ImGui::EndDragDropTarget();
    }

    void SceneEditorPanels::draw_inspector()
    {
        if (ImGui::Begin("Components"))
        {
            if (selected_entity.is_valid())
            {
                const Entity entity = selected_entity;

                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", get_entity_name(entity).c_str());
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_TRASH_ALT))
                {
                    defer([this, entity]()
                        {
                            destroy(entity);
                        });
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Delete entity");
                }

                draw_prefab_instance_section(entity);

                ImGui::Separator();
                EntityEditorUtils::edit_entity(world, entity.get_handle());
            }
            else
            {
                ImGui::TextDisabled("No entity selected");
            }
        }
        ImGui::End();
    }

    void SceneEditorPanels::draw_prefab_instance_section(const Entity& entity)
    {
        const PrefabInstanceComponent* instance_component = entity.get_component_if_exists<PrefabInstanceComponent>();
        if (instance_component == nullptr)
        {
            return;
        }

        const ref<PrefabAsset>& prefab = instance_component->prefab;
        const bool has_file = prefab != nullptr && !prefab->source_path.empty();

        std::string prefab_name = "In-memory prefab";
        if (has_file)
        {
            const std::optional<std::string> relative_path = AssetTypeRegistry::to_relative_path(prefab->source_path);
            prefab_name = relative_path.has_value() ? *relative_path : prefab->source_path.string();
        }

        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, PREFAB_INSTANCE_COLOR);
        ImGui::TextWrapped(EDITOR_ICON_PREFAB "  %s", prefab_name.c_str());
        ImGui::PopStyleColor();

        if (ImGui::Button("Select root"))
        {
            const std::vector<Entity> roots = PrefabUtils::get_instance_roots(entity);
            if (!roots.empty())
            {
                set_selected_entity(roots.front());
            }
        }
        ImGui::SameLine();
        if (disabled_button("Apply", has_file))
        {
            defer([entity]()
                {
                    if (PrefabUtils::apply_instance(entity))
                    {
                        LOG_MESSAGE("Editor> Prefab instance applied.");
                    }
                });
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Save this instance over the prefab file. Other instances are not updated.");
        }
        ImGui::SameLine();
        if (disabled_button("Revert", has_file))
        {
            defer([this, entity]()
                {
                    const std::vector<Entity> roots = PrefabUtils::revert_instance(entity);
                    if (!roots.empty())
                    {
                        set_selected_entity(roots.front());
                    }
                });
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Recreate the instance from the prefab file, keeping its placement.");
        }
        ImGui::SameLine();
        if (ImGui::Button("Unpack"))
        {
            defer([entity]()
                {
                    PrefabUtils::unpack_instance(entity);
                });
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Remove the link to the prefab.");
        }
    }

    void SceneEditorPanels::reparent(Entity entity, const Entity& new_parent)
    {
        if (!is_alive(world, entity) || !is_alive(world, new_parent))
        {
            return;
        }

        if (is_same_or_ancestor(world, entity, new_parent) || entity.get_parent_handle() == new_parent.get_handle())
        {
            return;
        }

        TransformComponent* transform = entity.get_component<TransformComponent>();
        const trs world_transform = transform->get_world_transform();

        entity.set_parent(new_parent.get_handle());
        transform->set_world_transform(world_transform);
    }

    void SceneEditorPanels::destroy(const Entity& entity)
    {
        if (!is_alive(world, entity))
        {
            return;
        }

        if (selected_entity.is_valid() && is_same_or_ancestor(world, entity, selected_entity))
        {
            set_selected_entity(Entity());
        }

        world->destroy_entity(entity.get_handle());
    }

    trs SceneEditorPanels::get_spawn_transform() const
    {
        const RendererHolderRootComponent* renderer_holder = world->get_root_component<RendererHolderRootComponent>();
        if (renderer_holder == nullptr)
        {
            return trs::identity;
        }

        const RenderCamera& camera = renderer_holder->camera;
        return trs(camera.position + camera.rotation * vec3(0.0f, 0.0f, -3.0f));
    }

    void SceneEditorPanels::defer(std::function<void()> action)
    {
        deferred_actions.push_back(std::move(action));
    }
}
