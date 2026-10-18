#include "ecs/editor/property_editor.h"

#include "ecs/world.h"
#include "ecs/base_components/name_component.h"
#include "ecs/serialization/serialization_meta.h"
#include "ecs/serialization/value_adapters.h"

#include "asset/asset_type_registry.h"

#include "core/editor_icons.h"
#include "core/imgui.h"
#include "core/log.h"

#include <imgui/imgui_internal.h>

namespace era_engine
{
    namespace
    {
        bool edit_object_impl(const rttr::instance& object, World* world, bool read_only);
        bool edit_value_impl(const char* label, rttr::variant& value, World* world, bool read_only);

        struct DisabledScope
        {
            DisabledScope(bool _disabled)
                : disabled(_disabled)
            {
                if (disabled)
                {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }
            }

            ~DisabledScope()
            {
                if (disabled)
                {
                    ImGui::PopStyleVar();
                    ImGui::PopItemFlag();
                }
            }

            bool disabled = false;
        };

        template <typename Type_>
        bool edit_scalar(const char* label, rttr::variant& value, ImGuiDataType data_type)
        {
            Type_ scalar = value.get_value<Type_>();
            if (ImGui::InputScalar(label, data_type, &scalar))
            {
                value = scalar;
                return true;
            }
            return false;
        }

        vec3 to_degrees(const vec3& radians)
        {
            return vec3(rad2deg(radians.x), rad2deg(radians.y), rad2deg(radians.z));
        }

        vec3 to_radians(const vec3& degrees)
        {
            return vec3(deg2rad(degrees.x), deg2rad(degrees.y), deg2rad(degrees.z));
        }

        bool edit_rotation(const char* label, quat& rotation)
        {
            vec3 euler = to_degrees(quat_to_euler(rotation));
            if (ImGui::DragFloat3(label, euler.data, 0.5f))
            {
                rotation = normalize(euler_to_quat(to_radians(euler)));
                return true;
            }
            return false;
        }

        bool edit_string(const char* label, std::string& text)
        {
            char buffer[256];
            strncpy_s(buffer, text.c_str(), _TRUNCATE);
            if (ImGui::InputText(label, buffer, sizeof(buffer)))
            {
                text = buffer;
                return true;
            }
            return false;
        }

        bool edit_leaf(const char* label, rttr::variant& value, bool& handled)
        {
            handled = true;
            const rttr::type type = value.get_type();

            if (type == rttr::type::get<bool>())
            {
                bool flag = value.get_value<bool>();
                if (ImGui::Checkbox(label, &flag))
                {
                    value = flag;
                    return true;
                }
                return false;
            }
            if (type == rttr::type::get<int8>())
                return edit_scalar<int8>(label, value, ImGuiDataType_S8);
            if (type == rttr::type::get<uint8>())
                return edit_scalar<uint8>(label, value, ImGuiDataType_U8);
            if (type == rttr::type::get<int16>())
                return edit_scalar<int16>(label, value, ImGuiDataType_S16);
            if (type == rttr::type::get<uint16>())
                return edit_scalar<uint16>(label, value, ImGuiDataType_U16);
            if (type == rttr::type::get<int32>())
                return edit_scalar<int32>(label, value, ImGuiDataType_S32);
            if (type == rttr::type::get<uint32>())
                return edit_scalar<uint32>(label, value, ImGuiDataType_U32);
            if (type == rttr::type::get<int64>())
                return edit_scalar<int64>(label, value, ImGuiDataType_S64);
            if (type == rttr::type::get<uint64>())
                return edit_scalar<uint64>(label, value, ImGuiDataType_U64);
            if (type == rttr::type::get<float>())
            {
                float scalar = value.get_value<float>();
                if (ImGui::DragFloat(label, &scalar, 0.01f))
                {
                    value = scalar;
                    return true;
                }
                return false;
            }
            if (type == rttr::type::get<double>())
            {
                double scalar = value.get_value<double>();
                if (ImGui::DragScalar(label, ImGuiDataType_Double, &scalar, 0.01f))
                {
                    value = scalar;
                    return true;
                }
                return false;
            }
            if (type == rttr::type::get<std::string>())
            {
                std::string text = value.get_value<std::string>();
                if (edit_string(label, text))
                {
                    value = text;
                    return true;
                }
                return false;
            }
            if (type == rttr::type::get<fs::path>())
            {
                std::string text = value.get_value<fs::path>().string();
                if (edit_string(label, text))
                {
                    value = fs::path(text);
                    return true;
                }
                return false;
            }
            if (type == rttr::type::get<vec2>())
            {
                vec2 vector = value.get_value<vec2>();
                if (ImGui::DragFloat2(label, vector.data, 0.01f))
                {
                    value = vector;
                    return true;
                }
                return false;
            }
            if (type == rttr::type::get<vec3>())
            {
                vec3 vector = value.get_value<vec3>();
                if (ImGui::DragFloat3(label, vector.data, 0.01f))
                {
                    value = vector;
                    return true;
                }
                return false;
            }
            if (type == rttr::type::get<vec4>())
            {
                vec4 vector = value.get_value<vec4>();
                if (ImGui::DragFloat4(label, vector.data, 0.01f))
                {
                    value = vector;
                    return true;
                }
                return false;
            }
            if (type == rttr::type::get<quat>())
            {
                quat rotation = value.get_value<quat>();
                if (edit_rotation(label, rotation))
                {
                    value = rotation;
                    return true;
                }
                return false;
            }
            if (type == rttr::type::get<trs>())
            {
                bool changed = false;
                if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
                {
                    trs transform = value.get_value<trs>();
                    changed |= ImGui::DragFloat3("Position", transform.position.data, 0.01f);
                    changed |= edit_rotation("Rotation", transform.rotation);
                    changed |= ImGui::DragFloat3("Scale", transform.scale.data, 0.01f);
                    if (changed)
                    {
                        value = transform;
                    }
                    ImGui::TreePop();
                }
                return changed;
            }
            if (type == rttr::type::get<mat3>() || type == rttr::type::get<mat4>())
            {
                ImGui::TextDisabled("%s: matrix", label);
                return false;
            }

            handled = false;
            return false;
        }

        std::string get_entity_label(const Entity& entity, Entity::Handle handle)
        {
            if (handle == Entity::null_handle)
            {
                return "None";
            }
            if (!entity.is_valid())
            {
                return std::format("Missing ({})", static_cast<uint32>(handle));
            }

            const NameComponent* name_component = entity.get_component_if_exists<NameComponent>();
            if (name_component != nullptr && name_component->name[0] != 0)
            {
                return std::format("{} ({})", name_component->name, static_cast<uint32>(handle));
            }
            return std::format("Entity ({})", static_cast<uint32>(handle));
        }

        void draw_reference_button(const std::string& text)
        {
            ImGui::Button(text.c_str(), ImVec2(ImGui::CalcItemWidth(), 0.0f));
        }

        bool draw_reference_tail(const char* label, bool read_only)
        {
            bool clear = false;
            ImGui::SameLine();
            if (!read_only && ImGui::SmallButton(ICON_FA_TIMES))
            {
                clear = true;
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Clear");
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(label);
            return clear;
        }

        bool edit_entity_reference(const char* label, rttr::variant& value, World* world, bool read_only)
        {
            const bool is_handle = value.get_type() == rttr::type::get<Entity::Handle>();

            Entity::Handle handle = Entity::null_handle;
            Entity entity;
            if (is_handle)
            {
                handle = value.get_value<Entity::Handle>();
                entity = world != nullptr && handle != Entity::null_handle ? world->get_entity(handle) : Entity();
            }
            else
            {
                entity = value.get_value<EntityPtr>().get();
                handle = entity.is_valid() ? entity.get_handle() : Entity::null_handle;
            }

            bool changed = false;

            draw_reference_button(get_entity_label(entity, handle));
            if (!read_only && ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EDITOR_PAYLOAD_ENTITY))
                {
                    const Entity::Handle dropped_handle = *static_cast<const Entity::Handle*>(payload->Data);
                    const Entity dropped = world != nullptr ? world->get_entity(dropped_handle) : Entity();
                    if (dropped.is_valid())
                    {
                        value = is_handle ? rttr::variant(dropped_handle) : rttr::variant(EntityPtr(dropped));
                        changed = true;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (draw_reference_tail(label, read_only))
            {
                value = is_handle ? rttr::variant(Entity::Handle(Entity::null_handle)) : rttr::variant(EntityPtr());
                changed = true;
            }
            return changed;
        }

        bool edit_asset_reference(const char* label, rttr::variant& value, const AssetTypeInfo& asset_type, bool read_only)
        {
            const ref<GameAsset> asset = asset_type.from_variant(value);

            std::string text = "None";
            if (asset != nullptr)
            {
                const std::optional<std::string> relative_path = AssetTypeRegistry::to_relative_path(asset->source_path);
                text = relative_path.has_value() ? *relative_path : (asset->source_path.empty() ? std::string("Runtime asset") : asset->source_path.filename().string());
            }

            bool changed = false;

            draw_reference_button(text);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Drop a %s file from Assets", asset_type.extension.c_str());
            }

            if (!read_only && ImGui::BeginDragDropTarget())
            {
                for (const char* payload_type : {EDITOR_ICON_FILE, EDITOR_ICON_PREFAB, EDITOR_ICON_MESH})
                {
                    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payload_type);
                    if (payload == nullptr)
                    {
                        continue;
                    }

                    const fs::path path = static_cast<const char*>(payload->Data);
                    if (path.extension().string() != asset_type.extension)
                    {
                        LOG_WARNING("Editor> '%s' is not a %s asset.", path.string().c_str(), asset_type.extension.c_str());
                        break;
                    }

                    const uint32 flags = asset != nullptr ? asset->flags : 0;
                    value = asset_type.to_variant(asset_type.load(path, flags));
                    changed = true;
                    break;
                }
                ImGui::EndDragDropTarget();
            }

            if (draw_reference_tail(label, read_only))
            {
                value = asset_type.to_variant(nullptr);
                changed = true;
            }
            return changed;
        }

        bool edit_enum(const char* label, rttr::variant& value)
        {
            const rttr::enumeration enumeration = value.get_type().get_enumeration();
            if (!enumeration.is_valid())
            {
                ImGui::TextDisabled("%s: unregistered enum", label);
                return false;
            }

            std::string current = enumeration.value_to_name(value).to_string();
            if (current.empty())
            {
                const EnumConverter* converter = ValueAdapters::find_enum_converter(value.get_type());
                current = converter != nullptr ? std::to_string(converter->to_integer(value)) : std::string("?");
            }

            bool changed = false;
            if (ImGui::BeginCombo(label, current.c_str()))
            {
                for (const rttr::string_view& name : enumeration.get_names())
                {
                    const std::string name_string = name.to_string();
                    if (ImGui::Selectable(name_string.c_str(), name_string == current))
                    {
                        value = enumeration.name_to_value(name);
                        changed = true;
                    }
                }
                ImGui::EndCombo();
            }
            return changed;
        }

        bool edit_adapted_value(const char* label, rttr::variant& value, const ValueAdapter& adapter, World* world, bool read_only)
        {
            const bool can_be_empty = adapter.wrap(rttr::variant()).is_valid();

            rttr::variant inner = adapter.unwrap(value);
            if (!inner.is_valid())
            {
                ImGui::TextDisabled("%s: none", label);
                if (!read_only)
                {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Set"))
                    {
                        value = adapter.wrap(adapter.make_default_inner());
                        return true;
                    }
                }
                return false;
            }

            bool changed = false;
            if (edit_value_impl(label, inner, world, read_only))
            {
                rttr::variant wrapped = adapter.wrap(inner);
                if (wrapped.is_valid())
                {
                    value = std::move(wrapped);
                    changed = true;
                }
            }

            if (can_be_empty && !read_only)
            {
                ImGui::SameLine();
                ImGui::PushID("reset");
                if (ImGui::SmallButton(ICON_FA_TIMES))
                {
                    value = adapter.wrap(rttr::variant());
                    changed = true;
                }
                ImGui::PopID();
            }
            return changed;
        }

        bool edit_sequence(const char* label, rttr::variant& value, World* world, bool read_only)
        {
            rttr::variant_sequential_view view = value.create_sequential_view();
            const bool resizable = view.is_dynamic() && !read_only;

            bool changed = false;
            const bool open = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_AllowItemOverlap, "%s [%u]", label, static_cast<uint32>(view.get_size()));

            if (resizable)
            {
                ImGui::SameLine();
                if (ImGui::SmallButton(ICON_FA_PLUS))
                {
                    changed |= view.set_size(view.get_size() + 1);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(ICON_FA_MINUS) && view.get_size() > 0)
                {
                    changed |= view.set_size(view.get_size() - 1);
                }
            }

            if (open)
            {
                for (size_t i = 0; i < view.get_size(); ++i)
                {
                    ImGui::PushID(static_cast<int>(i));
                    rttr::variant item = view.get_value(i).extract_wrapped_value();
                    const std::string item_label = std::format("[{}]", i);
                    if (edit_value_impl(item_label.c_str(), item, world, read_only))
                    {
                        changed |= view.set_value(i, item);
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
            return changed;
        }

        bool edit_value_impl(const char* label, rttr::variant& value, World* world, bool read_only)
        {
            DisabledScope disabled(read_only);

            const rttr::type type = value.get_type();

            bool handled = false;
            const bool leaf_changed = edit_leaf(label, value, handled);
            if (handled)
            {
                return leaf_changed;
            }

            if (type == rttr::type::get<Entity::Handle>() || type == rttr::type::get<EntityPtr>())
            {
                return edit_entity_reference(label, value, world, read_only);
            }

            if (const ValueAdapter* adapter = ValueAdapters::find(type))
            {
                return edit_adapted_value(label, value, *adapter, world, read_only);
            }

            if (const AssetTypeInfo* asset_type = AssetTypeRegistry::find(type))
            {
                return edit_asset_reference(label, value, *asset_type, read_only);
            }

            if (type.is_enumeration())
            {
                return edit_enum(label, value);
            }

            if (value.is_sequential_container())
            {
                return edit_sequence(label, value, world, read_only);
            }

            if (type.is_class() && !type.is_wrapper() && !type.get_properties().empty())
            {
                bool changed = false;
                if (ImGui::TreeNode(label))
                {
                    changed = edit_object_impl(rttr::instance(value), world, read_only);
                    ImGui::TreePop();
                }
                return changed;
            }

            ImGui::TextDisabled("%s: %s", label, type.get_name().to_string().c_str());
            return false;
        }

        bool edit_object_impl(const rttr::instance& object, World* world, bool read_only)
        {
            bool changed = false;

            const rttr::type type = object.get_derived_type();
            for (const rttr::property& property : type.get_properties())
            {
                if (property.is_static())
                {
                    continue;
                }

                const std::string name = property.get_name().to_string();
                const bool property_read_only = read_only || property.is_readonly();

                ImGui::PushID(name.c_str());

                rttr::variant value = property.get_value(object);
                if (edit_value_impl(name.c_str(), value, world, property_read_only) && !property_read_only)
                {
                    changed |= property.set_value(object, value);
                }

                if (!is_serializable(property) && ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Not saved");
                }

                ImGui::PopID();
            }
            return changed;
        }
    }

    bool PropertyEditor::edit_object(const rttr::instance& object, World* world)
    {
        return edit_object_impl(object, world, false);
    }

    bool PropertyEditor::edit_value(const char* label, rttr::variant& value, World* world, bool read_only)
    {
        return edit_value_impl(label, value, world, read_only) && !read_only;
    }
}
