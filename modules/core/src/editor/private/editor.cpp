// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "editor/editor.h"
#include "editor/system_calls.h"

#include "core/cpu_profiling.h"
#include "core/log.h"

#include "dx/dx_profiling.h"

#include "animation/animation.h"

#include "geometry/mesh.h"

#include "audio/audio.h"

#include "rendering/debug_visualization.h"

#include "terrain/terrain.h"
#include "terrain/proc_placement.h"
#include "terrain/grass.h"
#include "terrain/water.h"

#include "imgui/imgui_internal.h"

// #include "scripting/script.h"
// #include "scripting/native_scripting_linker.h"

#include "ecs/base_components/base_components.h"
#include "ecs/world.h"
#include "ecs/rendering/mesh_component.h"
#include "ecs/editor/entity_editor_utils.h"

// #include "physics/body_component.h"
// #include "physics/shape_component.h"
// #include "physics/core/physics.h"

#include <fontawesome/list.h>

#include <sstream>

namespace era_engine
{
    template <typename Component_, typename Member_>
    struct ComponentMemberUndo
    {
        ComponentMemberUndo(Member_& member, Member_ before, Entity entity, void (*callback)(Component_&, Member_, void*) = 0, void* user_data = 0)
        {
            this->entity = entity;
            this->byte_offset = (uint8*)&member - (uint8*)&entity.get_component<Component_>();
            this->before = before;
            this->callback = callback;
            this->user_data = user_data;
        }

        void toggle()
        {
            if (entity.valid())
            {
                Component_& comp = entity.get_component<Component_>();
                auto& member = *(Member_*)((uint8*)&comp + byte_offset);
                std::swap(member, before);

                if (callback)
                {
                    callback(comp, member, user_data);
                }
            }
        }

    private:
        Entity entity;
        uint64 byte_offset;

        Member_ before;

        void (*callback)(Component_&, Member_, void*) = 0;
        void* user_data = 0;
    };

    template <typename... Component_>
    struct ComponentUndo
    {
        ComponentUndo(Entity entity, Component_... before)
        {
            this->entity = entity;
            this->before = std::make_tuple(before...);
        }

        void toggle()
        {
            if (entity.valid())
            {
                std::apply([this](auto&... c)
                    {
                        (set(c), ...);
                    },
                    before);
            }
        }

    private:
        template <typename T>
        void set(T& c)
        {
            std::swap(entity.get_component<T>(), c);
        }

        Entity entity;

        std::tuple<Component_...> before;
    };

    template <typename Value_>
    struct SettingsUndo
    {
        SettingsUndo(Value_& value, Value_ before)
            : value(value), before(before) {}

        void toggle() { std::swap(value, before); }

    private:
        Value_& value;

        Value_ before;
    };

    struct EntityExistenceUndo
    {
        // TODO
        EntityExistenceUndo(ref<World> _world, Entity _entity)
            : world(_world), entity(_entity)
        {
            // size = serializeEntityToMemory(entity, buffer, sizeof(buffer));
        }

        ~EntityExistenceUndo() {}

        void toggle()
        {
            // if (entity.is_valid()) { deleteEntity(); }
            // else { restoreEntity(); }
        }

    private:
        void delete_entity()
        {
            // size = serializeEntityToMemory(entity, buffer, sizeof(buffer));
            // world->destroy_entity(entity);
        }

        void restore_entity()
        {
            // Entity::Handle place = entity.get_handle();
            // entity = world->try_create_entity_in_place(entity, "");
            // ASSERT(entity.get_handle() == place);

            // bool success = deserializeEntityFromMemory(entity, buffer, size);
            // ASSERT(success);
        }

        ref<World> world;
        Entity entity;
        uint8 buffer[1024];
        uint64 size;
    };

    template <typename Value_, typename Action_, typename... Args_>
    void EEditor::undoable(const char* undo_label, Value_ before, Value_& value,
        Args_... args)
    {
        if (ImGui::IsItemActive() && !ImGui::IsItemActiveLastFrame())
        {
            current_undo_buffer->as<Value_>() = before;
        }

        bool changed = ImGui::IsItemDeactivatedAfterEdit();
        if constexpr (std::is_enum_v<Value_> || std::is_integral_v<Value_>)
        {
            changed |= (!ImGui::IsItemActive() && (before != value));
        }

        if (changed)
        {
            std::stringstream s;
            s << std::setprecision(4) << std::boolalpha;
            s << undo_label << " from " << current_undo_buffer->as<Value_>() << " to " << value;
            Action_ action(value, current_undo_buffer->as<Value_>(), std::forward<Args_>(args)...);
            current_undo_stack->push_action(s.str().c_str(), action);
        }
    }

#define UNDOABLE_COMPONENT_SETTING(undo_label, val, command, ...)                \
    {                                                                            \
        using value_t = std::decay_t<decltype(val)>;                             \
        value_t before = val;                                                    \
        command;                                                                 \
        undoable<value_t, ComponentMemberUndo<component_t, value_t>>(undo_label, \
            before, val, selected_entity, __VA_ARGS__);                          \
    }

#define UNDOABLE_SETTING(undo_label, val, command)           \
    {                                                        \
        using value_t = std::decay_t<decltype(val)>;         \
        value_t before = val;                                \
        command;                                             \
        undoable<value_t, SettingsUndo<value_t>>(undo_label, \
            before, val);                                    \
    }

    void EEditor::update_selected_entity_ui_rotation()
    {
        if (selected_entity.is_valid())
        {
            quat rotation = quat::identity;

            if (TransformComponent* transform = selected_entity.get_component_if_exists<TransformComponent>())
            {
                rotation = transform->get_world_rotation();
            }

            selected_entity_euler_rotation = quat_to_euler(rotation);
        }
    }

    void EEditor::set_selected_entity(const Entity& entity)
    {
        selected_entity = entity;
        update_selected_entity_ui_rotation();
    }

    void EEditor::initialize(ref<EditorScene> _scene, MainRenderer* _renderer, EditorPanels* _editor_panels)
    {
        scene = _scene;
        renderer = _renderer;
        editor_panels = _editor_panels;
        camera_controller.initialize(&scene->camera);

        system_info = get_system_info();
    }

    bool EEditor::update(const UserInput& input, LDRRenderPass* ldr_render_pass, float dt)
    {
        CPU_PROFILE_BLOCK("Update editor");

        current_undo_stack = &undo_stacks[this->scene->mode > 0];
        current_undo_buffer = &undo_buffers[this->scene->mode > 0];

        Entity selected_entity_before = selected_entity;

        ref<World> world = scene->get_current_world();

        // Clear selected entity, if it became invalid (e.g. if it was deleted).
        if (!selected_entity.is_valid())
        {
            set_selected_entity({});
        }

        bool object_changed = false;
        object_changed |= handle_user_input(input, ldr_render_pass, dt);
        object_changed |= draw_scene_hierarchy();
        object_changed |= draw_main_menu_bar();
        draw_settings(dt);

        if (object_changed)
        {
            on_object_moved();
        }

        // This is triggered on undo.
        if (selected_entity != selected_entity_before)
        {
            set_selected_entity(selected_entity);
        }

        return object_changed;
    }

    void EEditor::render(LDRRenderPass* ldr_render_pass, float dt)
    {
        // TODO
    }

    static void draw_icons_window(bool& open)
    {
        if (open)
        {
            if (ImGui::Begin(ICON_FA_ICONS "  Icons", &open))
            {
                static ImGuiTextFilter filter;
                filter.Draw();

                if (ImGui::BeginChild("Icons List"))
                {
                    for (uint32 i = 0; i < arraysize(awesomeIcons); ++i)
                    {
                        ImGui::PushID(i);
                        if (filter.PassFilter(awesomeIconNames[i]))
                        {
                            ImGui::Text("%s: %s", awesomeIconNames[i], awesomeIcons[i]);
                            ImGui::SameLine();
                            if (ImGui::Button("Copy to clipboard"))
                            {
                                ImGui::SetClipboardText(awesomeIconNames[i]);
                            }
                        }
                        ImGui::PopID();
                    }
                }
                ImGui::EndChild();
            }
            ImGui::End();
        }
    }

    bool EEditor::draw_main_menu_bar()
    {
        static bool icons_window_open = false;
        static bool demo_window_open = false;
        static bool undo_window_open = false;
        static bool sound_editor_window_open = false;

        bool object_potentially_moved = false;

        bool controls_clicked = false;
        bool about_clicked = false;
        bool system_clicked = false;

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu(ICON_FA_FILE "  Project"))
            {
                char text_buffer[128];
                auto [undo_possible, undoName] = current_undo_stack->undo_possible();
                snprintf(text_buffer, sizeof(text_buffer), ICON_FA_UNDO " Undo %s", undo_possible ? undoName : "");
                if (ImGui::MenuItem(text_buffer, "Ctrl+Z", false, undo_possible))
                {
                    current_undo_stack->undo();
                    object_potentially_moved = true;
                }

                auto [redo_possible, redoName] = current_undo_stack->redo_possible();
                snprintf(text_buffer, sizeof(text_buffer), ICON_FA_REDO " Redo %s", redo_possible ? redoName : "");
                if (ImGui::MenuItem(text_buffer, "Ctrl+Y", false, redo_possible))
                {
                    current_undo_stack->redo();
                    object_potentially_moved = true;
                }

                if (ImGui::MenuItem(undo_window_open ? (ICON_FA_HISTORY "  Hide undo history") : (ICON_FA_HISTORY "  Show undo history")))
                {
                    undo_window_open = !undo_window_open;
                }
                ImGui::Separator();

                ImGui::Separator();
                if (ImGui::MenuItem(ICON_FA_TIMES "  Exit", "Esc"))
                {
                    PostQuitMessage(0);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_TOOLS "  Tools"))
            {
                if (ImGui::MenuItem(icons_window_open ? (ICON_FA_ICONS "  Hide available icons") : (ICON_FA_ICONS "  Show available icons")))
                {
                    icons_window_open = !icons_window_open;
                }

                if (ImGui::MenuItem(demo_window_open ? (ICON_FA_PUZZLE_PIECE "  Hide demo window") : (ICON_FA_PUZZLE_PIECE "  Show demo window")))
                {
                    demo_window_open = !demo_window_open;
                }

                ImGui::Separator();

                if (ImGui::MenuItem(dx_profiler_window_open ? (ICON_FA_CHART_BAR "  Hide GPU profiler") : (ICON_FA_CHART_BAR "  Show GPU profiler"), nullptr, nullptr, ENABLE_DX_PROFILING))
                {
                    dx_profiler_window_open = !dx_profiler_window_open;
                }

                if (ImGui::MenuItem(cpu_profiler_window_open ? (ICON_FA_CHART_LINE "  Hide CPU profiler") : (ICON_FA_CHART_LINE "  Show CPU profiler"), nullptr, nullptr, ENABLE_CPU_PROFILING))
                {
                    cpu_profiler_window_open = !cpu_profiler_window_open;
                }

                ImGui::Separator();

#ifdef ENABLE_MESSAGE_LOG
                if (ImGui::MenuItem(log_window_open ? (ICON_FA_CLIPBOARD_LIST "  Hide Console") : (ICON_FA_CLIPBOARD_LIST "  Show Console"), "Ctrl+L", nullptr, 1))
#else
                if (ImGui::MenuItem(log_window_open ? (ICON_FA_CLIPBOARD_LIST "  Hide Console") : (ICON_FA_CLIPBOARD_LIST "  Show Console"), "Ctrl+L", nullptr, 0))
#endif
                {
                    log_window_open = !log_window_open;
                }

                ImGui::Separator();

                if (ImGui::MenuItem(sound_editor_window_open ? (EDITOR_ICON_AUDIO "  Hide sound editor") : (EDITOR_ICON_AUDIO "  Show sound editor")))
                {
                    sound_editor_window_open = !sound_editor_window_open;
                }

                if (ImGui::MenuItem(editor_panels->mesh_editor.is_open() ? (EDITOR_ICON_MESH "  Hide mesh editor") : (EDITOR_ICON_MESH "  Show mesh editor")))
                {
                    if (editor_panels->mesh_editor.is_open())
                        editor_panels->mesh_editor.close();
                    else
                    {
                        editor_panels->mesh_editor.open();
                        editor_panels->mesh_editor.set_scene(scene);
                    }
                }

                ImGui::Separator();

                if (ImGui::MenuItem(ICON_FA_DESKTOP "  System"))
                {
                    system_clicked = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_QUESTION "  Help"))
            {
                // TODO: Docs
                if (ImGui::MenuItem(ICON_FA_MEMORY "  Repository"))
                    os::SystemCalls::open_url("https://github.com/EldarMuradau/EraEngine");

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_QUESTION "  About"))
            {
                if (ImGui::MenuItem(ICON_FA_QUESTION "  Author"))
                {
                    about_clicked = true;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();

        if (system_clicked)
        {
            ImGui::OpenPopup(ICON_FA_DESKTOP "  System");
        }

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(ICON_FA_DESKTOP "  System", 0, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Value("CPU", system_info.cpu_name.c_str());
            ImGui::Value("GPU", system_info.gpu_name.c_str());

            ImGui::Separator();

            ImGui::PopupOkButton();
            ImGui::EndPopup();
        }

        if (about_clicked)
        {
            ImGui::OpenPopup(ICON_FA_QUESTION "  About");
        }

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(ICON_FA_QUESTION "  About", 0, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Dummy(ImVec2(300.f, 1.f));
            ImGui::CenteredText("Era Engine");
            ImGui::CenteredText("by Eldar Muradov");

            ImGui::Separator();

            ImGui::PopupOkButton();
            ImGui::EndPopup();
        }

        if (demo_window_open)
        {
            ImGui::ShowDemoWindow(&demo_window_open);
        }

        draw_icons_window(icons_window_open);
        draw_sound_editor(sound_editor_window_open);

        return object_potentially_moved;
    }

    static BoundingBox get_object_bounding_box(Entity entity, bool apply_position)
    {
        BoundingBox aabb = entity.has_component<MeshComponent>() ? entity.get_component<MeshComponent>()->mesh->aabb : BoundingBox::from_center_radius(0.f, 1.f);

        if (TransformComponent* transform = entity.get_component_if_exists<TransformComponent>())
        {
            const trs& world_transform = transform->get_world_transform();

            aabb.min_corner *= world_transform.scale;
            aabb.max_corner *= world_transform.scale;

            if (apply_position)
            {
                aabb.min_corner += world_transform.position;
                aabb.max_corner += world_transform.position;
            }
        }

        return aabb;
    }

    void EEditor::render_childs(Entity& entity)
    {
        static int i = 0;

        ref<World> world = scene->get_current_world();

        auto childs = EntityContainer::get_childs(world.get(), entity.get_handle());

        if (!childs.empty())
        {
            auto iter = childs.begin();
            const auto& end = childs.end();
            for (; iter != end; ++iter)
            {
                Entity child = world->get_entity(*iter);
                NameComponent* name_component = child.get_component_if_exists<NameComponent>();
                ImGuiTreeNodeFlags flags = ((selected_entity == *iter) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
                if (name_component)
                {
                    if (ImGui::TreeNodeEx((void*)(uint32)(*iter), flags, name_component->name))
                    {
                        if (ImGui::IsItemClicked())
                        {
                            set_selected_entity(child);
                        }

                        render_childs(child);

                        ImGui::TreePop();
                    }
                }
                else
                {
                    if (ImGui::TreeNodeEx((void*)(uint32)(*iter), flags, std::to_string(i++).c_str()))
                    {
                        if (ImGui::IsItemClicked())
                        {
                            set_selected_entity(child);
                        }

                        render_childs(child);

                        ImGui::TreePop();
                    }
                }
            }
        }
    }

    static void draw_float_wrap(float wdt, const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", const ImGuiSliderFlags flags = 0)
    {
        static const uint32_t screen_edge_padding = 10;
        static bool needs_to_wrap = false;
        ImGuiIO& imgui_io = ImGui::GetIO();

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 position_cursor = imgui_io.MousePos;
            float position_left = static_cast<float>(screen_edge_padding);
            float position_right = static_cast<float>(wdt - screen_edge_padding);
            bool is_on_right_screen_edge = position_cursor.x >= position_right;
            bool is_on_left_screen_edge = position_cursor.x <= position_left;

            if (is_on_right_screen_edge)
            {
                position_cursor.x = position_left + 2;
                needs_to_wrap = true;
            }
            else if (is_on_left_screen_edge)
            {
                position_cursor.x = position_right - 2;
                needs_to_wrap = true;
            }
        }

        ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));
        ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, flags);
        ImGui::PopID();
    }

    static void show_vector3(const char* label, vec3& vector, float wdt)
    {
        const float label_indetation = 15.0f;

        const auto show_float = [wdt](vec3 axis, float* value)
        {
            const float label_float_spacing = 15.0f;
            const float step = 0.01f;

            // Label
            ImGui::TextUnformatted(axis.x == 1.0f ? "x" : axis.y == 1.0f ? "y"
                                                                         : "z");
            ImGui::SameLine(label_float_spacing);
            ImVec2 cp = ImGui::GetCursorScreenPos();
            vec2 pos_post_label = vec2(cp.x, cp.y);

            // Float
            ImGui::PushItemWidth(128.0f);
            ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));
            draw_float_wrap(wdt, "##no_label", value, step, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), "%.4f");
            ImGui::PopID();
            ImGui::PopItemWidth();

            // Axis color
            static const ImU32 color_x = IM_COL32(168, 46, 2, 255);
            static const ImU32 color_y = IM_COL32(112, 162, 22, 255);
            static const ImU32 color_z = IM_COL32(51, 122, 210, 255);
            static const vec2 size = vec2(4.0f, 19.0f);
            static const vec2 offset = vec2(5.0f, 4.0);
            pos_post_label += offset;
            ImRect axis_color_rect = ImRect(pos_post_label.x, pos_post_label.y, pos_post_label.x + size.x, pos_post_label.y + size.y);
            ImGui::GetWindowDrawList()->AddRectFilled(axis_color_rect.Min, axis_color_rect.Max, axis.x == 1.0f ? color_x : axis.y == 1.0f ? color_y
                                                                                                                                          : color_z);
        };

        ImGui::BeginGroup();
        ImGui::Indent(label_indetation);
        ImGui::TextUnformatted(label);
        ImGui::Unindent(label_indetation);
        show_float(vec3(1.0f, 0.0f, 0.0f), &vector.x);
        show_float(vec3(0.0f, 1.0f, 0.0f), &vector.y);
        show_float(vec3(0.0f, 0.0f, 1.0f), &vector.z);
        ImGui::EndGroup();
    };

    bool EEditor::draw_scene_hierarchy()
    {
        ref<World> world = scene->get_current_world();

        bool object_moved_by_widget = false;

        if (ImGui::Begin("Scene Hierarchy"))
        {
            if (ImGui::BeginChild("Outliner"))
            {
                world->view<NameComponent>()
                    .each([this, &world](Entity::Handle entity_handle, NameComponent& tag)
                        {
                            ImGui::PushID((uint32)entity_handle);
                            const char* name = tag.name;
                            Entity entity = world->get_entity(entity_handle);

                            Entity parent = world->get_entity(entity.get_parent_handle());

                            if (parent.is_valid())
                            {
                                ImGuiTreeNodeFlags flags = ((selected_entity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
                                if (ImGui::TreeNodeEx((void*)(uint32)entity_handle, flags, name))
                                {
                                    if (ImGui::IsItemClicked())
                                    {
                                        set_selected_entity(entity);
                                    }

                                    render_childs(entity);

                                    ImGui::TreePop();
                                }
                            }

                            bool entity_deleted = false;
                            if (ImGui::BeginPopupContextItem(name))
                            {
                                if (ImGui::MenuItem("Delete"))
                                {
                                    entity_deleted = true;
                                }

                                ImGui::EndPopup();
                            }

                            if (entity_deleted)
                            {
                                if (entity == selected_entity)
                                {
                                    set_selected_entity({});
                                }
                                // currentUndoStack->pushAction("entity deletion", entity_existence_undo(world, entity)); //TODO
                                world->destroy_entity(entity);
                            }
                            ImGui::PopID();
                        });
            }
            ImGui::EndChild();

            if (selected_entity.is_valid())
            {
                if (ImGui::Begin("Components"))
                {
                    ImGui::AlignTextToFramePadding();

                    ImGui::PushID((uint32)selected_entity.get_handle());

                    {
                        NameComponent* name_component = selected_entity.get_component_if_exists<NameComponent>();

                        NameComponent tag_before = *name_component;
                        ImGui::InputText("Name", name_component->name, sizeof(NameComponent::name));

                        if (ImGui::IsItemActive() && !ImGui::IsItemActiveLastFrame())
                        {
                            current_undo_buffer->as<NameComponent>() = tag_before;
                        }
                        if (ImGui::IsItemDeactivatedAfterEdit())
                        {
                            // TODO
                            // currentUndoStack->pushAction("entity name", component_undo<NameComponent>(selectedEntity, currentUndoBuffer->as<NameComponent>()));
                        }
                    }

                    ImGui::PopID();

                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_TRASH_ALT))
                    {
                        // currentUndoStack->pushAction("entity deletion", entity_existence_undo(world, selectedEntity));
                        world->destroy_entity(selected_entity);
                        set_selected_entity({});
                        object_moved_by_widget = true;
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Delete entity");
                    }

                    if (selected_entity.is_valid())
                    {
                        EntityEditorUtils::edit_entity(world.get(), selected_entity.get_handle());
                    }

                    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered())
                    {
                        ImGui::OpenPopup("CreateComponentPopup");
                    }

                    if (ImGui::BeginPopup("CreateComponentPopup"))
                    {
                        ImGui::Text("Create component");
                        ImGui::Separator();

                        // for (const auto& script : era_engine::dotnet::enative_scripting_linker::scriptTypes)
                        //{
                        //	if (ImGui::MenuItem(script.c_str()))
                        //	{
                        //		era_engine::dotnet::enative_scripting_linker::createScript((int)selectedEntity.get_handle(), script.c_str());
                        //	}
                        // }

                        ImGui::EndPopup();
                    }
                }
                ImGui::End();
            }
        }
        ImGui::End();

        return object_moved_by_widget;
    }

    void EEditor::on_object_moved()
    {
        if (selected_entity.is_valid())
        {
            // if (physics::px_cloth_component* cloth = selectedEntity.get_component_if_exists<physics::px_cloth_component>())
            //{
            //	cloth->translate(selectedEntity.get_component<TransformComponent>().transform.position);
            // }
        }
    }

    bool EEditor::handle_user_input(const UserInput& input, LDRRenderPass* ldr_render_pass, float dt)
    {
        ref<World> world = this->scene->get_current_world();

        // Returns true, if the user dragged an object using a gizmo.

        if (input.keyboard['F'].press_event && selected_entity.is_valid())
        {
            BoundingBox aabb = get_object_bounding_box(selected_entity, true);
            camera_controller.center_camera_on_object(aabb);
        }

        bool input_captured = camera_controller.update(input, renderer->render_width, renderer->render_height, dt);

        if (input_captured)
        {
            renderer->path_tracer.reset_rendering();
        }

        bool object_moved_by_gizmo = false;

        RenderCamera& camera = this->scene->is_pausable() ? this->scene->editor_camera : this->scene->camera;

        if (&camera != camera_controller.camera)
        {
            camera_controller.camera = &camera;
        }

        bool gizmo_drawn = false;

        if (!input_captured && !ImGui::IsAnyItemActive() && ImGui::IsKeyDown(KEY_SHIFT) && ImGui::IsKeyPressed('A'))
        {
            ImGui::OpenPopup("CreateEntityPopup");
            input_captured = true;
        }

        input_captured |= draw_entity_creation_popup();

        if (ImGui::BeginControlsWindow("##EntityControls", ImVec2(0.f, 0.f), ImVec2(20.f, 90.f)))
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

            if (ImGui::Button(ICON_FA_PLUS))
            {
                ImGui::OpenPopup("CreateEntityPopup");
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Create entity (Shift+A)");
            }
            input_captured |= draw_entity_creation_popup();

            ImGui::PopStyleColor();
        }
        ImGui::End();

        bool dragging_before = gizmo.dragging;

        if (selected_entity.is_valid())
        {
            if (TransformComponent* transform = selected_entity.get_component_if_exists<TransformComponent>())
            {
                // if (gizmo.manipulateTransformation(transform->get_world_transform(), camera, input, !inputCaptured, ldrRenderPass))
                {
                    /*if (auto rb = selectedEntity.get_component_if_exists<physics::DynamicBodyComponent>())
                    {
                        rb->manual_set_physics_position_and_rotation(transform->transform.position, transform->transform.rotation);
                    }
                    else if (auto rb = selectedEntity.get_component_if_exists<physics::StaticBodyComponent>())
                    {
                        rb->manual_set_physics_position_and_rotation(transform->transform.position, transform->transform.rotation);
                    }*/

                    // if (auto vehicle = physics::getVehicleComponent(selectedEntity))
                    //{
                    //	vehicle->setTransform(transform->position, transform->rotation);
                    // }

                    // if (physics::px_cloth_component* cloth = selectedEntity.getComponentIfExists<physics::px_cloth_component>())
                    //	cloth->translate(selectedEntity.getComponent<transform_component>().position);

                    update_selected_entity_ui_rotation();
                    input_captured = true;
                    object_moved_by_gizmo = true;
                }
                // else if (draggingBefore)
                {
                    // currentUndoStack->pushAction("object transform",
                    //	component_undo<TransformComponent>(selectedEntity, TransformComponent(selectedEntity.internal_data, gizmo.originalTransform)));
                }
            }
            else
            {
                gizmo.manipulate_nothing(camera, input, !input_captured, ldr_render_pass);
            }

            if (!input_captured && !ImGui::IsAnyItemActive())
            {
                if (ImGui::IsKeyPressed(KEY_BACKSPACE) || ImGui::IsKeyPressed(KEY_DELETE))
                {
                    // Delete entity.
                    // currentUndoStack->pushAction("entity deletion", entity_existence_undo(world, selectedEntity));
                    world->destroy_entity(selected_entity);
                    set_selected_entity({});
                    input_captured = true;
                    object_moved_by_gizmo = true;
                }
                else if (ImGui::IsKeyDown(KEY_CTRL) && ImGui::IsKeyPressed('D'))
                {
                    // Duplicate entity.
                    // Entity newEntity = world->copyEntity(selectedEntity);
                    // setSelectedEntity(newEntity);
                    // TODO
                    // inputCaptured = true;
                    // objectMovedByGizmo = true;
                }
            }
        }
        else
        {
            gizmo.manipulate_nothing(camera, input, !input_captured, ldr_render_pass);
        }

        float sim_controls_width = IMGUI_ICON_DEFAULT_SIZE * 3 + IMGUI_ICON_DEFAULT_SPACING * 2 + ImGui::GetStyle().WindowPadding.x * 2;

        if (ImGui::BeginControlsWindow("##SimulationControls", ImVec2(0.5f, 0.f), ImVec2(-sim_controls_width * 0.5f, 20.f)))
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

            // if (ImGui::IconButton(imgui_icon_play, imgui_icon_play, IMGUI_ICON_DEFAULT_SIZE, this->scene->is_playable()))
            //{
            //	setSelectedEntity({});
            //	forceStart();
            // }
            // ImGui::SameLine(0.f, IMGUI_ICON_DEFAULT_SPACING);
            if (ImGui::IconButton(IMGUI_ICON_PAUSE, IMGUI_ICON_PAUSE, IMGUI_ICON_DEFAULT_SIZE, this->scene->is_pausable()))
            {
                force_pause();
            }
            // ImGui::SameLine(0.f, IMGUI_ICON_DEFAULT_SPACING);
            // if (ImGui::IconButton(imgui_icon_stop, imgui_icon_stop, IMGUI_ICON_DEFAULT_SIZE, this->scene->is_stoppable()))
            //{
            //	forceStop();
            // }

            // scene = &this->scene->getCurrentScene();
            // cameraController.camera = &camera;

            current_undo_stack = &undo_stacks[this->scene->mode > 0];
            current_undo_buffer = &undo_buffers[this->scene->mode > 0];

            ImGui::PopStyleColor();
        }
        ImGui::End();

        if (!ImGui::IsAnyItemActive())
        {
            if (!input_captured && ImGui::IsKeyDown(KEY_CTRL) && ImGui::IsKeyPressed('Z'))
            {
                current_undo_stack->undo();
                input_captured = true;
                object_moved_by_gizmo = true;
            }
            if (!input_captured && ImGui::IsKeyDown(KEY_CTRL) && ImGui::IsKeyPressed('Y'))
            {
                current_undo_stack->redo();
                input_captured = true;
                object_moved_by_gizmo = true;
            }
            if (!input_captured && ImGui::IsKeyDown(KEY_CTRL) && ImGui::IsKeyPressed('S'))
            {
                serialize_to_file();
                input_captured = true;
                ImGui::GetIO().KeysDown['S'] = false; // Hack: Window does not get notified of inputs due to the file dialog.
            }
            if (!input_captured && ImGui::IsKeyDown(KEY_CTRL) && ImGui::IsKeyPressed('O'))
            {
                deserialize_from_file();
                input_captured = true;
                ImGui::GetIO().KeysDown['O'] = false; // Hack: Window does not get notified of inputs due to the file dialog.
            }
            if (!input_captured && ImGui::IsKeyDown(KEY_CTRL) && ImGui::IsKeyPressed('L'))
            {
                log_window_open = !log_window_open;
                input_captured = true;
            }
        }

        if (!input_captured && input.mouse.left.click_event)
        {
            if (input.keyboard[KEY_CTRL].down)
            {
                vec3 dir = -camera.generate_world_space_ray(input.mouse.rel_x, input.mouse.rel_y).direction;
                vec3 before_dir = this->scene->sun.direction;
                this->scene->sun.direction = dir;
                // currentUndoStack->pushAction("sun direction", settings_undo<vec3>(this->scene->sun.direction, beforeDir));
                input_captured = true;
            }
            else
            {
                if (renderer->hovered_object_id != -1)
                {
                    set_selected_entity(world->get_entity(Entity::Handle{renderer->hovered_object_id}));
                }
                else
                {
                    set_selected_entity({});
                }
            }
            input_captured = true;
        }

        return object_moved_by_gizmo;
    }

    bool EEditor::draw_entity_creation_popup()
    {
        ref<World> world = scene->get_current_world();
        RenderCamera& camera = this->scene->camera;

        bool clicked = false;

        if (ImGui::BeginPopup("CreateEntityPopup"))
        {
            if (ImGui::MenuItem("Point light", "P") || ImGui::IsKeyPressed('P'))
            {
                // Entity pl = world->create_entity("Point light")
                //	.add_component<PointLightComponent>(
                //		vec3(1.f, 1.f, 1.f),
                //		1.f,
                //		10.f,
                //		true,
                //		2048u
                //	);

                // TransformComponent& transform = pl.get_component<TransformComponent>();
                // transform.transform.position = camera.position + camera.rotation * vec3(0.f, 0.f, -3.f);

                // currentUndoStack->pushAction("entity creation", entity_existence_undo(world, pl));

                // setSelectedEntity(pl);
                clicked = true;
            }

            if (ImGui::MenuItem("Spot light", "S") || ImGui::IsKeyPressed('S'))
            {
                // Entity sl = world->create_entity("Spot light")
                //	.add_component<SpotLightComponent>(
                //		vec3(1.f, 1.f, 1.f),
                //		1.f,
                //		25.f,
                //		deg2rad(20.f),
                //		deg2rad(30.f),
                //		true,
                //		2048u
                //	);

                // TransformComponent& transform = sl.get_component<TransformComponent>();
                // transform.transform.position = camera.position + camera.rotation * vec3(0.f, 0.f, -3.f);

                // currentUndoStack->pushAction("entity creation", entity_existence_undo(world, sl));

                // setSelectedEntity(sl);
                clicked = true;
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Empty", "E") || ImGui::IsKeyPressed('E'))
            {
                auto empty = world->create_entity("Empty");
                TransformComponent* transform = empty.get_component<TransformComponent>();
                transform->set_world_position(camera.position + camera.rotation * vec3(0.f, 0.f, -3.f));

                // currentUndoStack->pushAction("entity creation", entity_existence_undo(world, empty));

                set_selected_entity(empty);
                clicked = true;
            }

            if (clicked)
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        return clicked;
    }

    void EEditor::serialize_to_file()
    {
        // serializeSceneToYAMLFile(*scene, renderer->settings);
    }

    bool EEditor::deserialize_from_file()
    {
        ////std::string environmentName;
        ////if (deserializeSceneFromYAMLFile(*scene, renderer->settings, environmentName))
        ////{
        ////	scene->stop();

        ////	setSelectedEntity({});
        ////	scene->environment.setFromTexture(environmentName);
        ////	scene->environment.forceUpdate(this->scene->sun.direction);
        ////	renderer->pathTracer.resetRendering();

        ////	return true;
        ////}
        return false;
    }

    bool EEditor::deserialize_from_current_file(const fs::path& path)
    {
        // std::string environmentName;
        // if (deserializeSceneFromCurrentYAMLFile(path, *scene, renderer->settings, environmentName))
        //{
        //	scene->stop();

        //	setSelectedEntity({});
        //	scene->environment.setFromTexture(environmentName);
        //	scene->environment.forceUpdate(this->scene->sun.direction);
        //	renderer->pathTracer.resetRendering();

        //	return true;
        //}
        return false;
    }

    bool EEditor::edit_camera(RenderCamera& camera)
    {
        bool result = false;
        if (ImGui::BeginTree("Camera"))
        {
            UNDOABLE_SETTING("position", camera.position,
                result |= ImGui::DragFloat3("Position", camera.position.data));

            if (ImGui::BeginProperties())
            {
                UNDOABLE_SETTING("field of view", camera.vertical_fov,
                    result |= ImGui::PropertySliderAngle("Field of view", camera.vertical_fov, 1.f, 150.f));
                UNDOABLE_SETTING("near plane", camera.near_plane,
                    result |= ImGui::PropertyDrag("Near plane", camera.near_plane, 0.01f, 0.f));

                bool infinite_farplane = camera.far_plane < 0.f;
                UNDOABLE_SETTING(
                    "infinite far plane", camera.far_plane,
                    if (ImGui::PropertyCheckbox("Infinite far plane", infinite_farplane)) {
                        if (!infinite_farplane)
                        {
                            camera.far_plane = (camera.far_plane == -1.f) ? 500.f : -camera.far_plane;
                        }
                        else
                        {
                            camera.far_plane = -camera.far_plane;
                        }
                        result = true;
                    });
                if (!infinite_farplane)
                {
                    UNDOABLE_SETTING("far plane", camera.far_plane,
                        result |= ImGui::PropertyDrag("Far plane", camera.far_plane, 0.1f, 0.f));
                }

                ImGui::EndProperties();
            }

            ImGui::EndTree();
        }
        return result;
    }

    bool EEditor::edit_tonemapping(TonemapSettings& tonemap)
    {
        bool result = false;
        if (ImGui::BeginTree("Tonemapping"))
        {
            ImGui::PlotLines(
                "",
                [](void* data, int idx)
                {
                    float t = idx * 0.01f;
                    TonemapSettings& aces = *(TonemapSettings*)data;
                    return aces.tonemap(t);
                },
                &tonemap, 100, 0, 0, 0.f, 1.f, ImVec2(250.f, 250.f));

            if (ImGui::BeginProperties())
            {
                UNDOABLE_SETTING("shoulder strength", tonemap.A,
                    result |= ImGui::PropertySlider("Shoulder strength", tonemap.A, 0.f, 1.f));
                UNDOABLE_SETTING("linear strength", tonemap.B,
                    result |= ImGui::PropertySlider("Linear strength", tonemap.B, 0.f, 1.f));
                UNDOABLE_SETTING("linear angle", tonemap.C,
                    result |= ImGui::PropertySlider("Linear angle", tonemap.C, 0.f, 1.f));
                UNDOABLE_SETTING("toe strength", tonemap.D,
                    result |= ImGui::PropertySlider("Toe strength", tonemap.D, 0.f, 1.f));
                UNDOABLE_SETTING("tone numerator", tonemap.E,
                    result |= ImGui::PropertySlider("Tone numerator", tonemap.E, 0.f, 1.f));
                UNDOABLE_SETTING("toe denominator", tonemap.F,
                    result |= ImGui::PropertySlider("Toe denominator", tonemap.F, 0.f, 1.f));
                UNDOABLE_SETTING("linear white", tonemap.linear_white,
                    result |= ImGui::PropertySlider("Linear white", tonemap.linear_white, 0.f, 100.f));
                UNDOABLE_SETTING("exposure", tonemap.exposure,
                    result |= ImGui::PropertySlider("Exposure", tonemap.exposure, -3.f, 3.f));
                ImGui::EndProperties();
            }

            ImGui::EndTree();
        }
        return result;
    }

    bool EEditor::edit_sun_shadow_parameters(DirectionalLight& sun)
    {
        bool result = false;
        if (ImGui::BeginTree("Sun"))
        {
            if (ImGui::BeginProperties())
            {
                UNDOABLE_SETTING("sun intensity", sun.intensity,
                    result |= ImGui::PropertySlider("Intensity", sun.intensity, 0.f, 1000.f));
                UNDOABLE_SETTING("sun color", sun.color,
                    result |= ImGui::PropertyColorWheel("Color", sun.color));

                UNDOABLE_SETTING("sun shadow resolution", sun.shadow_dimensions,
                    result |= ImGui::PropertyDropdownPowerOfTwo("Shadow resolution", 128, 2048, sun.shadow_dimensions));
                UNDOABLE_SETTING("stabilize", sun.stabilize,
                    result |= ImGui::PropertyCheckbox("Stabilize", sun.stabilize));
                UNDOABLE_SETTING("negative z-offset", sun.negative_z_offset,
                    result |= ImGui::PropertyDrag("Negative z-offset", sun.negative_z_offset, 0.5f, 0.f));

                UNDOABLE_SETTING("cascade count", sun.num_shadow_cascades,
                    result |= ImGui::PropertySlider("Cascade count", sun.num_shadow_cascades, 1, 4));

                const float min_cascade_distance = 0.f, max_cascade_distance = 300.f;
                const float min_bias = 0.f, max_bias = 0.0015f;
                const float min_blend = 0.f, max_blend = 10.f;
                if (sun.num_shadow_cascades == 1)
                {
                    UNDOABLE_SETTING("cascade distance", sun.cascade_distances.x,
                        result |= ImGui::PropertySlider("Distance", sun.cascade_distances.x, min_cascade_distance, max_cascade_distance));
                    UNDOABLE_SETTING("cascade bias", sun.bias.x,
                        result |= ImGui::PropertySlider("Bias", sun.bias.x, min_bias, max_bias, "%.6f"));
                    UNDOABLE_SETTING("cascade blend distances", sun.blend_distances.x,
                        result |= ImGui::PropertySlider("Blend distances", sun.blend_distances.x, min_blend, max_blend, "%.6f"));
                }
                else if (sun.num_shadow_cascades == 2)
                {
                    UNDOABLE_SETTING("cascade distance", sun.cascade_distances.xy,
                        result |= ImGui::PropertySlider("Distance", sun.cascade_distances.xy, min_cascade_distance, max_cascade_distance));
                    UNDOABLE_SETTING("cascade bias", sun.bias.xy,
                        result |= ImGui::PropertySlider("Bias", sun.bias.xy, min_bias, max_bias, "%.6f"));
                    UNDOABLE_SETTING("cascade blend distances", sun.blend_distances.xy,
                        result |= ImGui::PropertySlider("Blend distances", sun.blend_distances.xy, min_blend, max_blend, "%.6f"));
                }
                else if (sun.num_shadow_cascades == 3)
                {
                    UNDOABLE_SETTING("cascade distance", sun.cascade_distances.xyz,
                        result |= ImGui::PropertySlider("Distance", sun.cascade_distances.xyz, min_cascade_distance, max_cascade_distance));
                    UNDOABLE_SETTING("cascade bias", sun.bias.xyz,
                        result |= ImGui::PropertySlider("Bias", sun.bias.xyz, min_bias, max_bias, "%.6f"));
                    UNDOABLE_SETTING("cascade blend distances", sun.blend_distances.xyz,
                        result |= ImGui::PropertySlider("Blend distances", sun.blend_distances.xyz, min_blend, max_blend, "%.6f"));
                }
                else if (sun.num_shadow_cascades == 4)
                {
                    UNDOABLE_SETTING("cascade distance", sun.cascade_distances,
                        result |= ImGui::PropertySlider("Distance", sun.cascade_distances, min_cascade_distance, max_cascade_distance));
                    UNDOABLE_SETTING("cascade bias", sun.bias,
                        result |= ImGui::PropertySlider("Bias", sun.bias, min_bias, max_bias, "%.6f"));
                    UNDOABLE_SETTING("cascade blend distances", sun.blend_distances,
                        result |= ImGui::PropertySlider("Blend distances", sun.blend_distances, min_blend, max_blend, "%.6f"));
                }

                ImGui::EndProperties();
            }

            if (ImGui::BeginTree("Show##ShowShadowMaps"))
            {
                for (uint32 i = 0; i < sun.num_shadow_cascades; ++i)
                {
                    auto vp = sun.shadow_map_viewports[i];
                    ImGui::Image(RenderResources::shadow_map, 0, 0, ImVec2(vp.x, vp.y), ImVec2(vp.x + vp.z, vp.y + vp.w));
                }
                ImGui::EndTree();
            }

            ImGui::EndTree();
        }
        return result;
    }

    bool EEditor::edit_ao(bool& enable, HBAOSettings& settings, const ref<DXTexture>& ao_texture)
    {
        bool result = false;
        if (ImGui::BeginProperties())
        {
            UNDOABLE_SETTING("enable HBAO", enable,
                result |= ImGui::PropertyCheckbox("Enable HBAO", enable));
            if (enable)
            {
                UNDOABLE_SETTING("num AO rays", settings.num_rays,
                    result |= ImGui::PropertySlider("Num rays", settings.num_rays, 1, 16));
                UNDOABLE_SETTING("max num steps per AO ray", settings.max_num_steps_per_ray,
                    result |= ImGui::PropertySlider("Max num steps per ray", settings.max_num_steps_per_ray, 1, 16));
                UNDOABLE_SETTING("AO radius", settings.radius,
                    result |= ImGui::PropertySlider("Radius", settings.radius, 0.f, 1.f, "%.3fm"));
                UNDOABLE_SETTING("AO strength", settings.strength,
                    result |= ImGui::PropertySlider("Strength", settings.strength, 0.f, 2.f));
            }
            ImGui::EndProperties();
        }
        if (enable && ao_texture && ImGui::BeginTree("Show##ShowAO"))
        {
            ImGui::Image(ao_texture);
            ImGui::EndTree();
        }
        return result;
    }

    bool EEditor::edit_sss(bool& enable, SSSSettings& settings, const ref<DXTexture>& sss_texture)
    {
        bool result = false;
        if (ImGui::BeginProperties())
        {
            UNDOABLE_SETTING("enable SSS", enable,
                result |= ImGui::PropertyCheckbox("Enable SSS", enable));
            if (enable)
            {
                UNDOABLE_SETTING("num SSS iterations", settings.num_steps,
                    result |= ImGui::PropertySlider("Num iterations", settings.num_steps, 1, 64));
                UNDOABLE_SETTING("SSS ray distance", settings.ray_distance,
                    result |= ImGui::PropertySlider("Ray distance", settings.ray_distance, 0.05f, 3.f, "%.3fm"));
                UNDOABLE_SETTING("SSS thickness", settings.thickness,
                    result |= ImGui::PropertySlider("Thickness", settings.thickness, 0.05f, 1.f, "%.3fm"));
                UNDOABLE_SETTING("SSS max distance from camera", settings.max_distance_from_camera,
                    result |= ImGui::PropertySlider("Max distance from camera", settings.max_distance_from_camera, 5.f, 1000.f, "%.3fm"));
                UNDOABLE_SETTING("SSS distance fadeout range", settings.distance_fadeout_range,
                    result |= ImGui::PropertySlider("Distance fadeout range", settings.distance_fadeout_range, 1.f, 5.f, "%.3fm"));
                UNDOABLE_SETTING("SSS border fadeout", settings.border_fadeout,
                    result |= ImGui::PropertySlider("Border fadeout", settings.border_fadeout, 0.f, 0.5f));
            }
            ImGui::EndProperties();
        }
        if (enable && sss_texture && ImGui::BeginTree("Show##ShowSSS"))
        {
            ImGui::Image(sss_texture);
            ImGui::EndTree();
        }
        return result;
    }

    bool EEditor::edit_ssr(bool& enable, SSRSettings& settings, const ref<DXTexture>& ssr_texture)
    {
        bool result = false;
        if (ImGui::BeginProperties())
        {
            UNDOABLE_SETTING("enable SSR", enable,
                result |= ImGui::PropertyCheckbox("Enable SSR", enable));
            if (enable)
            {
                UNDOABLE_SETTING("num SSR iterations", settings.num_steps,
                    result |= ImGui::PropertySlider("Num iterations", settings.num_steps, 1, 1024));
                UNDOABLE_SETTING("SSR max distance", settings.max_distance,
                    result |= ImGui::PropertySlider("Max distance", settings.max_distance, 5.f, 1000.f, "%.3fm"));
                UNDOABLE_SETTING("SSR min stride", settings.min_stride,
                    result |= ImGui::PropertySlider("Min stride", settings.min_stride, 1.f, 50.f, "%.3fm"));
                UNDOABLE_SETTING("SSR max stride", settings.max_stride,
                    result |= ImGui::PropertySlider("Max stride", settings.max_stride, settings.min_stride, 50.f, "%.3fm"));
            }
            ImGui::EndProperties();
        }
        if (enable && ssr_texture && ImGui::BeginTree("Show##ShowSSR"))
        {
            ImGui::Image(ssr_texture);
            ImGui::EndTree();
        }
        return result;
    }

    bool EEditor::edit_taa(bool& enable, TAASettings& settings, const ref<DXTexture>& velocity_texture)
    {
        bool result = false;
        if (ImGui::BeginProperties())
        {
            UNDOABLE_SETTING("enable TAA", enable,
                result |= ImGui::PropertyCheckbox("Enable TAA", enable));
            if (enable)
            {
                UNDOABLE_SETTING("TAA jitter strength", settings.camera_jitter_strength,
                    result |= ImGui::PropertySlider("Jitter strength", settings.camera_jitter_strength));
            }
            ImGui::EndProperties();
        }
        if (enable && velocity_texture && ImGui::BeginTree("Show##ShowTAA"))
        {
            ImGui::Image(velocity_texture);
            ImGui::EndTree();
        }
        return result;
    }

    bool EEditor::edit_bloom(bool& enable, BloomSettings& settings, const ref<DXTexture>& bloom_texture)
    {
        bool result = false;
        if (ImGui::BeginProperties())
        {
            UNDOABLE_SETTING("enable bloom", enable,
                result |= ImGui::PropertyCheckbox("Enable bloom", enable));
            if (enable)
            {
                UNDOABLE_SETTING("bloom threshold", settings.threshold,
                    result |= ImGui::PropertySlider("Bloom threshold", settings.threshold, 0.5f, 100.f));
                UNDOABLE_SETTING("bloom strength", settings.strength,
                    result |= ImGui::PropertySlider("Bloom strength", settings.strength));
            }
            ImGui::EndProperties();
        }
        if (enable && bloom_texture && ImGui::BeginTree("Show##ShowBloom"))
        {
            ImGui::Image(bloom_texture);
            ImGui::EndTree();
        }
        return result;
    }

    bool EEditor::edit_sharpen(bool& enable, SharpenSettings& settings)
    {
        bool result = false;
        if (ImGui::BeginProperties())
        {
            UNDOABLE_SETTING("enable sharpen", enable,
                result |= ImGui::PropertyCheckbox("Enable sharpen", enable));
            if (enable)
            {
                UNDOABLE_SETTING("sharpen strength", settings.strength,
                    result |= ImGui::PropertySlider("Sharpen strength", settings.strength));
            }
            ImGui::EndProperties();
        }
        return result;
    }

    volatile bool paused = false;

    void EEditor::force_start()
    {
        this->scene->play();
        undo_stacks[1].reset();
        set_selected_entity({});

        // for (auto [entityHandle, rigidbody, transform] : scene->get_current_world()->group(components_group<physics::DynamicBodyComponent, TransformComponent>).each())
        //{
        //	rigidbody.manual_set_physics_position_and_rotation(transform.transform.position, transform.transform.rotation);
        // }

        // for (auto [entityHandle, rigidbody, transform] : scene->get_current_world()->group(components_group<physics::StaticBodyComponent, TransformComponent>).each())
        //{
        //	rigidbody.manual_set_physics_position_and_rotation(transform.transform.position, transform.transform.rotation);
        // }

        // if (!paused)
        //{
        //	app->linker->start();
        // }
        // else
        //{
        //	paused = false;
        // }
    }

    void EEditor::force_pause()
    {
        this->scene->pause();
        paused = true;
    }

    void EEditor::force_stop()
    {
        this->scene->stop();
        this->scene->environment.force_update(this->scene->sun.direction);
        set_selected_entity({});
        // app->linker->reload_src();
        // physics::PhysicsEngine::get_physics_core()->reset_actors_velocity_and_inertia();
        paused = false;
        this->scene->editor_camera.set_position_and_rotation(vec3(0.0f), quat::identity);
    }

    void EEditor::visualize_physics(LDRRenderPass* ldr_render_pass) const
    {
        /*using namespace physx;

        if (!renderPhysicsShapes)
        {
            return;
        }

        auto scene = physics::PhysicsEngine::get_physics_core()->get_scene();
        const physx::PxRenderBuffer& rb = scene->getRenderBuffer();

        for (physx::PxU32 i = 0; i < rb.getNbPoints(); i++)
        {
            const physx::PxDebugPoint& point = rb.getPoints()[i];
            renderPoint(physx::createVec3(point.pos), vec4(1.0f), ldrRenderPass);
        }

        for (physx::PxU32 i = 0; i < rb.getNbLines(); i++)
        {
            const physx::PxDebugLine& line = rb.getLines()[i];
            renderLine(physx::createVec3(line.pos0), physx::createVec3(line.pos1), vec4(1.0f), ldrRenderPass);
        }*/
    }

    void EEditor::draw_settings(float dt)
    {
        ref<World> world = scene->get_current_world();

        if (ImGui::Begin("Settings"))
        {
            PathTracer& path_tracer = renderer->path_tracer;

            ImGui::Text("%.3f ms, %u FPS", dt * 1000.f, (uint32)(1.f / dt));

            if (ImGui::BeginProperties())
            {
                UNDOABLE_SETTING("time scale", this->scene->timestep_scale,
                    ImGui::PropertySlider("Time scale", this->scene->timestep_scale));

                UNDOABLE_SETTING(
                    "renderer mode", renderer->mode,
                    if (ImGui::PropertyDropdown("Renderer mode", renderer_mode_names, RENDERER_MODE_COUNT, (uint32&)renderer->mode)) {
                        path_tracer.reset_rendering();
                    });

                DXMemoryUsage memory_usage = dx_context.get_memory_usage();

                ImGui::PropertyValue("Video memory usage", "%u / %uMB", memory_usage.currently_used, memory_usage.available);
                ImGui::PropertyValue("Render queue command lists count", "%u", dx_context.render_queue.total_num_command_lists);

                UNDOABLE_SETTING("aspect ratio", renderer->aspect_ratio_mode,
                    ImGui::PropertyDropdown("Aspect ratio", aspect_ratio_names, ASPECT_RATIO_MODE_COUNT, (uint32&)renderer->aspect_ratio_mode));

                UNDOABLE_SETTING("static shadow map caching", enable_static_shadow_map_caching,
                    ImGui::PropertyCheckbox("Static shadow map caching", enable_static_shadow_map_caching));

                UNDOABLE_SETTING("shadow map caching", renderer->settings.cache_shadow_map,
                    ImGui::PropertyCheckbox("Shadow map caching", renderer->settings.cache_shadow_map));

                ImGui::EndProperties();
            }

            if (!this->scene->is_pausable())
            {
                edit_camera(this->scene->camera);
            }
            else
            {
                edit_camera(this->scene->editor_camera);
            }
            edit_tonemapping(renderer->settings.tonemap_settings);
            edit_sun_shadow_parameters(this->scene->sun);

            if (ImGui::BeginTree("Post processing"))
            {
                if (renderer->spec.allow_ao)
                {
                    edit_ao(renderer->settings.enable_ao, renderer->settings.ao_settings, renderer->get_ao_result());
                    ImGui::Separator();
                }
                if (renderer->spec.allow_sss)
                {
                    edit_sss(renderer->settings.enable_sss, renderer->settings.sss_settings, renderer->get_sss_result());
                    ImGui::Separator();
                }
                if (renderer->spec.allow_ssr)
                {
                    edit_ssr(renderer->settings.enable_ssr, renderer->settings.ssr_settings, renderer->get_ssr_result());
                    ImGui::Separator();
                }
                if (renderer->spec.allow_taa)
                {
                    edit_taa(renderer->settings.enable_taa, renderer->settings.taa_settings, renderer->get_screen_velocities());
                    ImGui::Separator();
                }
                if (renderer->spec.allow_bloom)
                {
                    edit_bloom(renderer->settings.enable_bloom, renderer->settings.bloom_settings, renderer->get_bloom_result());
                    ImGui::Separator();
                }
                if (renderer->spec.allow_dlss)
                {

#if ENABLE_DLSS

                    if (ImGui::BeginProperties())
                    {
                        // For now it's only fsr2 wrapper. I do not have rtx gpu now :(((
                        ImGui::PropertyCheckbox("Enable FSR 2.0", renderer->settings.enable_dlss);

                        ImGui::EndProperties();
                    }

                    if (renderer->settings.enable_dlss)
                    {
                        if (!renderer->dlss_inited)
                        {
                            renderer->dlss_adapter.initialize(renderer);
                        }
                    }
                    else
                    {
                        renderer->settings.tonemap_settings = renderer->settings.default_tonemap_settings;
                    }
                    ImGui::Separator();

#endif
                }
                edit_sharpen(renderer->settings.enable_sharpen, renderer->settings.sharpen_settings);

                ImGui::EndTree();
            }

            if (ImGui::BeginTree("Environment"))
            {
                auto& environment = this->scene->environment;

                if (ImGui::BeginProperties())
                {
                    AssetHandle handle = (environment.sky) ? environment.sky->handle : AssetHandle{};
                    if (ImGui::PropertyAssetHandle("Texture source", EDITOR_ICON_IMAGE_HDR, handle, "Make proc"))
                    {
                        if (handle)
                        {
                            environment.set_from_texture(get_path_from_asset_handle(handle));
                        }
                        else
                        {
                            environment.set_to_procedural(this->scene->sun.direction);
                        }
                    }

                    UNDOABLE_SETTING("sky intensity", environment.sky_intensity,
                        ImGui::PropertySlider("Sky intensity", environment.sky_intensity, 0.f, 2.f));
                    UNDOABLE_SETTING("GI intensity", environment.global_illumination_intensity,
                        ImGui::PropertySlider("GI intensity", environment.global_illumination_intensity, 0.f, 2.f));

                    EnvironmentGIMode temp_mode = environment.gi_mode;

                    ImGui::PropertyDropdown("GI mode", environment_gi_mode_names, 1 + 1, (uint32&)temp_mode);

                    if (temp_mode != environment.gi_mode)
                    {
                        struct ChangeGIModeData
                        {
                            EnvironmentGIMode mode;
                            EnvironmentGIMode& dest_mode;
                        } data{temp_mode, this->scene->environment.gi_mode};
                        main_thread_job_queue.create_job<ChangeGIModeData>([](ChangeGIModeData& data, JobHandle job)
                                                 {
                                                     data.dest_mode = data.mode;
                                                 },
                                                 data)
                            .submit_now();
                    }

                    ImGui::EndProperties();
                }

                if (environment.gi_mode == ENVIRONMENT_GI_RAYTRACED)
                {
                    if (ImGui::BeginTree("Light probe"))
                    {
                        auto& grid = environment.light_probe_grid;

                        if (ImGui::BeginProperties())
                        {
                            UNDOABLE_SETTING("visualize probes", grid.visualize_probes,
                                ImGui::PropertyCheckbox("Visualize probes", grid.visualize_probes));
                            UNDOABLE_SETTING("visualize rays", grid.visualize_rays,
                                ImGui::PropertyCheckbox("Visualize rays", grid.visualize_rays));

                            UNDOABLE_SETTING("auto rotate rays", grid.auto_rotate_rays,
                                ImGui::PropertyCheckbox("Auto rotate rays", grid.auto_rotate_rays));
                            if (!grid.auto_rotate_rays)
                            {
                                grid.rotate_rays = ImGui::PropertyButton("Rotate", "Go");
                            }

                            ImGui::EndProperties();
                        }

                        if (ImGui::BeginTree("Irradiance"))
                        {
                            if (ImGui::BeginProperties())
                            {
                                UNDOABLE_SETTING("irradiance UI scale", grid.irradiance_ui_scale,
                                    ImGui::PropertySlider("Scale", grid.irradiance_ui_scale, 0.1f, 20.f));
                                ImGui::EndProperties();
                            }
                            if (grid.irradiance)
                            {
                                ImGui::Image(grid.irradiance, (uint32)(grid.irradiance->width * grid.irradiance_ui_scale));
                            }
                            ImGui::EndTree();
                        }
                        if (ImGui::BeginTree("Depth"))
                        {
                            if (ImGui::BeginProperties())
                            {
                                UNDOABLE_SETTING("depth UI scale", grid.depth_ui_scale,
                                    ImGui::PropertySlider("Scale", grid.depth_ui_scale, 0.1f, 20.f));
                                ImGui::EndProperties();
                            }
                            if (grid.depth)
                            {
                                ImGui::Image(grid.depth, (uint32)(grid.depth->width * grid.depth_ui_scale));
                            }
                            ImGui::EndTree();
                        }
                        if (ImGui::BeginTree("Raytraced radiance"))
                        {
                            if (ImGui::BeginProperties())
                            {
                                UNDOABLE_SETTING("raytraced radiance UI scale", grid.raytraced_radiance_ui_scale,
                                    ImGui::PropertySlider("Scale", grid.raytraced_radiance_ui_scale, 0.1f, 20.f));
                                ImGui::EndProperties();
                            }
                            if (grid.raytraced_radiance)
                            {
                                ImGui::Image(grid.raytraced_radiance, (uint32)(grid.raytraced_radiance->width * grid.raytraced_radiance_ui_scale));
                            }
                            ImGui::EndTree();
                        }

                        ImGui::EndTree();
                    }
                }
                ImGui::EndTree();
            }

            /*if (ImGui::BeginTree("Physics (PhysX)"))
            {
                if (ImGui::BeginProperties())
                {
                    const auto& physics_ref = physics::PhysicsEngine::get_physics_core();
                    float frame_rate = physics_ref->frame_rate;

                    ImGui::PropertyInput("Frame rate", frame_rate);
                    if (frame_rate != physics_ref->frame_rate)
                    {
                        physics_ref->frame_rate = frame_rate;
                    }

                    if (physics_ref->frame_rate < 30)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.f, 0.f, 1.f));
                        ImGui::PropertyValue("", "Low frame rate");
                        ImGui::PopStyleColor();
                    }

                    uint32 nbaa = physics_ref->nb_active_actors.load(std::memory_order_relaxed);
                    uint32 nba = physics_ref->actors_map.size();
                    ImGui::PropertyValue("Number of active actors", std::to_string(nbaa).c_str());
                    ImGui::PropertyValue("Number of actors", std::to_string(nba).c_str());
                    if (physics::PhysicsEngine::get_physics_core()->is_gpu())
                    {
                        ImGui::PropertyValue("Broad phase", "GPU");
                    }
                    else
                    {
                        ImGui::PropertyValue("Broad phase", "PABP (CPU)");
                    }
                    ImGui::EndProperties();
                }

                ImGui::EndTree();
            }*/

            if (ImGui::BeginTree("Audio"))
            {
                bool change = false;
                if (ImGui::BeginProperties())
                {
                    const float max_volume = 3.f;
                    UNDOABLE_SETTING("master volume", master_audio_settings.volume,
                        change |= ImGui::PropertySlider("Master volume", master_audio_settings.volume, 0.f, max_volume));

                    ImGui::PropertySeparator();

                    for (uint32 i = 0; i < SOUND_TYPE_COUNT; ++i)
                    {
                        UNDOABLE_SETTING(sound_type_names[i], sound_type_volumes[i],
                            change |= ImGui::PropertySlider(sound_type_names[i], sound_type_volumes[i], 0.f, 1.f));
                    }

                    ImGui::PropertySeparator();

                    UNDOABLE_SETTING("toggle reverb", master_audio_settings.reverb_enabled,
                        ImGui::PropertyCheckbox("Reverb enabled", master_audio_settings.reverb_enabled));

                    if (master_audio_settings.reverb_enabled)
                    {
                        UNDOABLE_SETTING("reverb preset", master_audio_settings.reverb_preset,
                            change |= ImGui::PropertyDropdown("Reverb preset", reverb_preset_names, REVERB_PRESET_COUNT, (uint32&)master_audio_settings.reverb_preset));
                    }

                    ImGui::EndProperties();
                }
                ImGui::EndTree();
            }

            if (renderer->mode == RENDERER_MODE_PATHTRACED)
            {
                if (ImGui::BeginProperties())
                {
                    auto& settings = path_tracer.settings;

                    UNDOABLE_SETTING("max recursion depth", settings.recursion_depth,
                        ImGui::PropertySlider("Max recursion depth", settings.recursion_depth, 0, settings.max_recursion_depth - 1));
                    UNDOABLE_SETTING("start russion roulette after", settings.start_russian_roulette_after,
                        ImGui::PropertySlider("Start russian roulette after", settings.start_russian_roulette_after, 0, settings.recursion_depth));
                    UNDOABLE_SETTING("use thin lens camera", settings.use_thin_lens_camera,
                        ImGui::PropertyCheckbox("Use thin lens camera", settings.use_thin_lens_camera));
                    if (settings.use_thin_lens_camera)
                    {
                        UNDOABLE_SETTING("focal length", settings.focal_length,
                            ImGui::PropertySlider("Focal length", settings.focal_length, 0.5f, 50.f));
                        UNDOABLE_SETTING("f-number", settings.f_number,
                            ImGui::PropertySlider("F-Number", settings.f_number, 1.f, 128.f));
                    }
                    UNDOABLE_SETTING("use real materials", settings.use_real_materials,
                        ImGui::PropertyCheckbox("Use real materials", settings.use_real_materials));
                    UNDOABLE_SETTING("enable direct lighting", settings.enable_direct_lighting,
                        ImGui::PropertyCheckbox("Enable direct lighting", settings.enable_direct_lighting));
                    if (settings.enable_direct_lighting)
                    {
                        UNDOABLE_SETTING("light intensity scale", settings.light_intensity_scale,
                            ImGui::PropertySlider("Light intensity scale", settings.light_intensity_scale, 0.f, 50.f));
                        UNDOABLE_SETTING("point light radius", settings.point_light_radius,
                            ImGui::PropertySlider("Point light radius", settings.point_light_radius, 0.01f, 1.f));

                        UNDOABLE_SETTING("multiple importance sampling", settings.multiple_importance_sampling,
                            ImGui::PropertyCheckbox("Multiple importance sampling", settings.multiple_importance_sampling));
                    }

                    ImGui::EndProperties();
                }
            }
            else
            {
                if (ImGui::BeginTree("Particle systems"))
                {
                    // editFireParticleSystem(app->fireParticleSystem);
                    // editBoidParticleSystem(app->boidParticleSystem);

                    ImGui::EndTree();
                }
            }
        }

        ImGui::End();
    }

    void edit_texture(const char* name, ref<DXTexture>& tex, uint32 load_flags)
    {
        AssetHandle asset = {};
        if (tex)
        {
            asset = tex->handle;
        }

        if (ImGui::PropertyTextureAssetHandle(name, EDITOR_ICON_IMAGE, asset, tex))
        {
            fs::path path = get_path_from_asset_handle(asset);
            fs::path relative = fs::relative(path, fs::current_path());
            if (auto new_tex = load_texture_from_file_async(path, load_flags))
            {
                tex = new_tex;
            }
        }
    }

    void edit_mesh(const char* name, ref<MultiMesh>& mesh, uint32 load_flags)
    {
        AssetHandle asset = {};
        if (mesh)
        {
            asset = mesh->handle;
        }

        if (ImGui::PropertyAssetHandle(name, EDITOR_ICON_MESH, asset))
        {
            fs::path path = get_path_from_asset_handle(asset);
            fs::path relative = fs::relative(path, fs::current_path());
            if (auto new_mesh = import_mesh_from_file(path, load_flags))
            {
                mesh = new_mesh;
            }
        }
    }

    void edit_material(const ref<PBRMaterial>& material)
    {
        if (ImGui::BeginProperties())
        {
            AssetHandle dummy = {};

            edit_texture("Albedo", material->albedo, IMAGE_LOAD_FLAGS_DEFAULT);
            edit_texture("Normal", material->normal, IMAGE_LOAD_FLAGS_DEFAULT_NONCOLOR);
            edit_texture("Roughness", material->roughness, IMAGE_LOAD_FLAGS_DEFAULT_NONCOLOR);
            edit_texture("Metallic", material->metallic, IMAGE_LOAD_FLAGS_DEFAULT_NONCOLOR);

            ImGui::PropertyColor("Emission", material->emission);
            ImGui::PropertyColor("Albedo tint", material->albedo_tint);
            ImGui::PropertyDropdown("Shader", pbr_material_shader_names, PBR_MATERIAL_SHADER_COUNT, (uint32&)material->shader);
            ImGui::PropertySlider("UV scale", material->uv_scale, 0.0f, 150.0f);
            ImGui::PropertySlider("Translucency", material->translucency);

            if (!material->roughness)
            {
                ImGui::PropertySlider("Roughness override", material->roughness_override);
            }

            if (!material->metallic)
            {
                ImGui::PropertySlider("Metallic override", material->metallic_override);
            }

            ImGui::EndProperties();
        }
    }

    void edit_submesh_transform(trs* transform)
    {
        ImGui::Drag("Position", transform->position, 0.1f);
        vec3 selected_entity_euler_rotation = quat_to_euler(transform->rotation);

        if (ImGui::Drag("Rotation", selected_entity_euler_rotation, 0.1f))
        {
            transform->rotation = euler_to_quat(selected_entity_euler_rotation);
        }

        ImGui::Drag("Scale", transform->scale, 0.1f);
    }

}