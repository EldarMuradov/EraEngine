#include "editor/vehicle_import_panel.h"

#include <core/imgui.h>
#include <core/log.h>

#include <editor/file_dialog.h>

#include <asset/asset_type_registry.h>

#include <geometry/mesh.h>

#include <ecs/world.h>
#include <ecs/base_components/transform_component.h>

#include <format>

namespace era_engine
{
    static const char* forward_axis_names[] = {"+Z", "-Z", "+X", "-X"};
    static const char* drive_type_names[] = {"All wheels", "Front wheels", "Rear wheels"};
    static const char* unit_names[] = {"Auto", "Meters", "Centimeters"};
    static const float unit_scales[] = {0.0f, 1.0f, 0.01f};

    VehicleImportPanel::VehicleImportPanel(World* _world)
        : world(_world)
    {
    }

    void VehicleImportPanel::open()
    {
        is_open = true;
    }

    void VehicleImportPanel::draw()
    {
        using namespace physics;

        if (!is_open)
        {
            return;
        }

        if (!ImGui::Begin(ICON_FA_CAR "  Vehicle Import", &is_open))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Browse..."))
        {
            const std::string path = open_file_dialog("FBX model", "fbx");
            if (!path.empty())
            {
                model_path = path;
                load_model();
            }
        }
        ImGui::SameLine();
        ImGui::TextWrapped("%s", model_path.empty() ? "No model" : model_path.c_str());

        if (source == nullptr)
        {
            ImGui::TextWrapped("%s", status.c_str());
            ImGui::End();
            return;
        }

        bool needs_analyze = false;
        bool needs_rebuild = false;

        int units = 0;
        for (int i = 0; i < IM_ARRAYSIZE(unit_scales); ++i)
        {
            units = settings.model_scale == unit_scales[i] ? i : units;
        }
        if (ImGui::Combo("Units", &units, unit_names, IM_ARRAYSIZE(unit_names)))
        {
            settings.model_scale = unit_scales[units];
            needs_analyze = true;
        }
        if (settings.model_scale == 0.0f)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(scale %.2f)", setup.model_scale);
        }

        int forward_axis = static_cast<int>(settings.forward_axis);
        if (ImGui::Combo("Model forward", &forward_axis, forward_axis_names, IM_ARRAYSIZE(forward_axis_names)))
        {
            settings.forward_axis = static_cast<VehicleForwardAxis>(forward_axis);
            needs_analyze = true;
        }

        int drive_type = static_cast<int>(settings.drive_type);
        if (ImGui::Combo("Drive", &drive_type, drive_type_names, IM_ARRAYSIZE(drive_type_names)))
        {
            settings.drive_type = static_cast<VehicleDriveType>(drive_type);
            needs_rebuild = true;
        }

        if (ImGui::DragFloat("Mass (kg)", &settings.mass, 10.0f, 100.0f, 50000.0f, "%.0f"))
        {
            needs_rebuild = true;
        }

        ImGui::Separator();
        ImGui::Text("Parts (check wheel parts)");

        const uint32 nb_axles = static_cast<uint32>(setup.config.axles.size());

        if (ImGui::BeginTable("##vehicle_parts", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 220.0f)))
        {
            ImGui::TableSetupColumn("Wheel", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Door", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Steer", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Size (m)");
            ImGui::TableHeadersRow();

            for (uint32 i = 0; i < setup.parts.size(); ++i)
            {
                VehicleImportPart& part = setup.parts[i];

                std::string slot = "-";
                for (const VehicleImportWheel& wheel : setup.wheels)
                {
                    if (std::find(wheel.parts.begin(), wheel.parts.end(), i) != wheel.parts.end())
                    {
                        slot = VehicleImportUtils::get_wheel_label(wheel.wheel_id, nb_axles);
                    }
                    if (std::find(wheel.hub_parts.begin(), wheel.hub_parts.end(), i) != wheel.hub_parts.end())
                    {
                        slot = "hub " + VehicleImportUtils::get_wheel_label(wheel.wheel_id, nb_axles);
                    }
                }
                for (const VehicleImportDoor& door : setup.doors)
                {
                    if (std::find(door.parts.begin(), door.parts.end(), i) != door.parts.end())
                    {
                        slot = "door " + door.label;
                    }
                }
                if (part.is_steering_wheel)
                {
                    slot = "steering";
                }

                const vec3 size = part.aabb.max_corner - part.aabb.min_corner;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::Checkbox("##is_wheel", &part.is_wheel))
                {
                    part.is_door = part.is_door && !part.is_wheel;
                    part.is_steering_wheel = part.is_steering_wheel && !part.is_wheel;
                    needs_rebuild = true;
                }
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##is_door", &part.is_door))
                {
                    part.is_wheel = part.is_wheel && !part.is_door;
                    part.is_steering_wheel = part.is_steering_wheel && !part.is_door;
                    needs_rebuild = true;
                }
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##is_steering_wheel", &part.is_steering_wheel))
                {
                    part.is_wheel = part.is_wheel && !part.is_steering_wheel;
                    part.is_door = part.is_door && !part.is_steering_wheel;
                    needs_rebuild = true;
                }
                ImGui::PopID();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(part.name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(slot.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%.2f x %.2f x %.2f", size.x, size.y, size.z);
            }

            ImGui::EndTable();
        }

        if (!setup.error.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.3f, 1.0f), "%s", setup.error.c_str());
        }
        else
        {
            ImGui::Separator();
            ImGui::Text("Wheels");
            for (const VehicleImportWheel& wheel : setup.wheels)
            {
                ImGui::BulletText("%s: center (%.2f, %.2f, %.2f), radius %.3f, width %.3f",
                    VehicleImportUtils::get_wheel_label(wheel.wheel_id, nb_axles).c_str(),
                    wheel.center.x,
                    wheel.center.y,
                    wheel.center.z,
                    wheel.radius,
                    wheel.width);
            }

            const VehicleChassisConfig& chassis = setup.config.chassis;
            ImGui::Text("Chassis box: center (%.2f, %.2f, %.2f), half extents (%.2f, %.2f, %.2f)",
                chassis.collision_center.x,
                chassis.collision_center.y,
                chassis.collision_center.z,
                chassis.collision_half_extents.x,
                chassis.collision_half_extents.y,
                chassis.collision_half_extents.z);
            ImGui::Text("Center of mass height: %.2f", chassis.center_of_mass.y);

            for (const VehicleImportDoor& door : setup.doors)
            {
                ImGui::BulletText("Door %s: hinge (%.2f, %.2f, %.2f), %zu parts", door.label.c_str(), door.hinge.x, door.hinge.y, door.hinge.z, door.parts.size());
            }
            if (!setup.steering_wheel.parts.empty())
            {
                const VehicleImportSteeringWheel& steering = setup.steering_wheel;
                ImGui::BulletText("Steering wheel: center (%.2f, %.2f, %.2f), axis (%.2f, %.2f, %.2f)", steering.center.x, steering.center.y, steering.center.z, steering.axis.x, steering.axis.y, steering.axis.z);
            }
            ImGui::TextDisabled("Fine-tune everything in the W4VehicleComponent inspector after creation.");

            ImGui::Separator();
            ImGui::InputText("Name", vehicle_name, sizeof(vehicle_name));
            if (ImGui::Button(ICON_FA_CAR "  Create vehicle and prefab"))
            {
                create_vehicle();
            }
        }

        if (!status.empty())
        {
            ImGui::TextWrapped("%s", status.c_str());
        }

        if (needs_analyze)
        {
            analyze();
        }
        else if (needs_rebuild)
        {
            VehicleImportUtils::rebuild(setup, settings);
        }

        ImGui::End();
    }

    void VehicleImportPanel::load_model()
    {
        setup = {};
        status.clear();

        source = import_mesh_from_file(model_path, MESH_CREATION_FLAGS_DEFAULT);

        if (source == nullptr || !source->model_asset.has_value() || source->model_asset->meshes.empty())
        {
            source = nullptr;
            status = "Failed to import the model.";
            return;
        }

        if (!AssetTypeRegistry::to_relative_path(model_path).has_value())
        {
            status = "Warning: the model is outside resources/assets, the prefab will not be loadable.";
        }

        const fs::path stem = fs::path(model_path).stem();
        strncpy_s(vehicle_name, stem.string().c_str(), _TRUNCATE);

        setup = physics::VehicleImportUtils::analyze(*source->model_asset, settings);
    }

    void VehicleImportPanel::analyze()
    {
        using namespace physics;

        std::vector<std::tuple<bool, bool, bool>> flags;
        for (const VehicleImportPart& part : setup.parts)
        {
            flags.emplace_back(part.is_wheel, part.is_door, part.is_steering_wheel);
        }

        setup = VehicleImportUtils::analyze(*source->model_asset, settings);

        if (flags.size() == setup.parts.size())
        {
            for (size_t i = 0; i < flags.size(); ++i)
            {
                std::tie(setup.parts[i].is_wheel, setup.parts[i].is_door, setup.parts[i].is_steering_wheel) = flags[i];
            }
            VehicleImportUtils::rebuild(setup, settings);
        }
    }

    void VehicleImportPanel::create_vehicle()
    {
        const fs::path output_directory = fs::path(model_path).parent_path();
        const std::string name = vehicle_name;

        if (name.empty())
        {
            status = "Name is empty.";
            return;
        }

        Entity vehicle = physics::VehicleImportUtils::create_vehicle(world, source, setup, output_directory, name);
        if (!vehicle.is_valid())
        {
            status = "Failed to create the vehicle, see log.";
            return;
        }

        status = std::format("Created '{}' and saved {}.eprefab next to the model.", name, name);
    }
}
