#include "ecs/editor/entity_editor_utils.h"
#include "ecs/world.h"
#include "ecs/component.h"
#include "ecs/base_components/base_components.h"
#include "ecs/editor/property_editor.h"
#include "ecs/serialization/component_registry.h"

#include "core/editor_icons.h"

#include "imgui/imgui_internal.h"

namespace era_engine
{
    static void draw_add_component_button(Entity& entity)
    {
        if (ImGui::Button(ICON_FA_PLUS "  Add component", ImVec2(-1.0f, 0.0f)))
        {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            static ImGuiTextFilter filter;
            if (ImGui::IsWindowAppearing())
            {
                filter.Clear();
                ImGui::SetKeyboardFocusHere();
            }
            filter.Draw("##filter", 250.0f);
            ImGui::Separator();

            std::vector<const ComponentTypeInfo*> component_types = ComponentRegistry::get_all();
            std::sort(component_types.begin(), component_types.end(), [](const ComponentTypeInfo* a, const ComponentTypeInfo* b)
                {
                    return a->type.get_name() < b->type.get_name();
                });

            for (const ComponentTypeInfo* info : component_types)
            {
                if (!info->is_serializable() || info->get(entity) != nullptr)
                {
                    continue;
                }

                const std::string name = info->type.get_name().to_string();
                if (filter.PassFilter(name.c_str()) && ImGui::Selectable(name.c_str()))
                {
                    info->add(entity);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
    }

    void EntityEditorUtils::edit_entity(World* world, const Entity::Handle handle)
    {
        Entity entity = world->get_entity(handle);
        if (!entity.is_valid())
        {
            return;
        }

        ImGui::PushID(static_cast<int>(static_cast<uint32>(handle)));

        std::vector<const ComponentTypeInfo*> component_types;
        std::vector<std::string> unregistered_types;

        for (auto [storage_id, storage] : world->get_registry().storage())
        {
            if (!storage.contains(handle))
            {
                continue;
            }

            if (const ComponentTypeInfo* info = ComponentRegistry::find_by_storage_id(storage_id))
            {
                component_types.push_back(info);
            }
            else
            {
                unregistered_types.emplace_back(storage.type().name());
            }
        }

        const rttr::type transform_type = rttr::type::get<TransformComponent>();
        std::sort(component_types.begin(), component_types.end(), [&transform_type](const ComponentTypeInfo* a, const ComponentTypeInfo* b)
            {
                if ((a->type == transform_type) != (b->type == transform_type))
                {
                    return a->type == transform_type;
                }
                return a->type.get_name() < b->type.get_name();
            });

        const ComponentTypeInfo* component_to_remove = nullptr;

        for (const ComponentTypeInfo* info : component_types)
        {
            Component* component = info->get(entity);
            if (component == nullptr)
            {
                continue;
            }

            const std::string name = info->type.get_name().to_string();
            ImGui::PushID(name.c_str());

            const bool open = ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            if (!info->is_serializable() && ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Runtime-only component, not saved to prefabs");
            }

            if (ImGui::BeginPopupContextItem("ComponentContext"))
            {
                if (ImGui::MenuItem("Remove component", nullptr, false, info->type != transform_type))
                {
                    component_to_remove = info;
                }
                ImGui::EndPopup();
            }

            if (open)
            {
                ImGui::Indent();
                PropertyEditor::edit_object(*component, world);
                ImGui::Unindent();
            }

            ImGui::PopID();
        }

        for (const std::string& name : unregistered_types)
        {
            ImGui::TextDisabled("%s (not registered)", name.c_str());
        }

        if (component_to_remove != nullptr)
        {
            component_to_remove->remove(entity);
        }

        ImGui::Separator();
        draw_add_component_button(entity);

        ImGui::PopID();
    }

    void EntityEditorUtils::show_vector3(const char* label, vec3& vector, float wdt)
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
    }

    void EntityEditorUtils::draw_float_wrap(float wdt, const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, const ImGuiSliderFlags flags)
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
}