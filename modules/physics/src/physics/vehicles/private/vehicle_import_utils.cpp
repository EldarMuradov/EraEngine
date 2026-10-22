#include "physics/vehicles/vehicle_import_utils.h"
#include "physics/vehicles/w4_vehicle_component.h"

#include <core/log.h>

#include <asset/game_asset.h>
#include <asset/pbr_material_desc.h>

#include <geometry/mesh.h>

#include <ecs/world.h>
#include <ecs/base_components/transform_component.h>
#include <ecs/rendering/mesh_component.h>
#include <ecs/prefab/prefab_serializer.h>

#include <algorithm>
#include <format>
#include <numeric>

namespace era_engine::physics
{
    static constexpr float OVERLAP_EPSILON = 0.01f;
    static constexpr float CENTIMETER_MODEL_LENGTH = 20.0f;
    static constexpr uint32 NB_HULL_DIRECTIONS = 96;
    static constexpr float DOOR_OPEN_ANGLE = 70.0f;
    static constexpr float FRONT_EDGE_BAND = 0.02f;

    // Fibonacci sphere.
    static const std::vector<vec3>& get_hull_directions()
    {
        static const std::vector<vec3> directions = []()
        {
            std::vector<vec3> result;
            const float golden_angle = M_PI * (3.0f - sqrt(5.0f));
            for (uint32 i = 0; i < NB_HULL_DIRECTIONS; ++i)
            {
                const float y = 1.0f - 2.0f * (i + 0.5f) / NB_HULL_DIRECTIONS;
                const float radius = sqrt(1.0f - y * y);
                const float angle = golden_angle * i;
                result.push_back(vec3(cos(angle) * radius, y, sin(angle) * radius));
            }
            return result;
        }();
        return directions;
    }

    static bool contains_any(const std::string& text, std::initializer_list<const char*> tokens)
    {
        for (const char* token : tokens)
        {
            if (text.find(token) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    static std::string to_lower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return text;
    }

    static bool is_wheel_name(const std::string& name)
    {
        return contains_any(name, {"wheel", "whl", "tire", "tyre", "rim"}) && !contains_any(name, {"steer", "spare", "arch", "well", "fender", "cover"});
    }

    static bool is_door_name(const std::string& name)
    {
        return contains_any(name, {"door"}) && !contains_any(name, {"frame", "jamb", "sill", "seal"});
    }

    static bool is_steering_wheel_name(const std::string& name)
    {
        return contains_any(name, {"steering"}) || (contains_any(name, {"steer"}) && contains_any(name, {"wheel", "whl"}));
    }

    static std::vector<vec3> merge_support_points(const std::vector<VehicleImportPart>& parts, const std::vector<uint32>& indices, const vec3& offset)
    {
        const std::vector<vec3>& directions = get_hull_directions();

        std::vector<vec3> result;
        for (size_t d = 0; d < directions.size(); ++d)
        {
            float best_distance = -FLT_MAX;
            vec3 best_point = vec3::zero;
            for (uint32 index : indices)
            {
                const float distance = dot(parts[index].support_points[d], directions[d]);
                if (distance > best_distance)
                {
                    best_distance = distance;
                    best_point = parts[index].support_points[d];
                }
            }

            const vec3 point = best_point + offset;
            const bool is_duplicate = std::any_of(result.begin(), result.end(), [&](const vec3& other)
                {
                    return length(other - point) < OVERLAP_EPSILON;
                });
            if (!is_duplicate)
            {
                result.push_back(point);
            }
        }
        return result;
    }

    static bool is_wheel_shape(const BoundingBox& aabb, const BoundingBox& model_aabb)
    {
        const vec3 extents = aabb.max_corner - aabb.min_corner;
        const float diameter = std::max(extents.y, extents.z);
        const float model_height = model_aabb.max_corner.y - model_aabb.min_corner.y;
        const float model_mid_height = (model_aabb.max_corner.y + model_aabb.min_corner.y) * 0.5f;

        return diameter > 0.0f &&
               std::abs(extents.y - extents.z) < 0.2f * diameter &&
               extents.x < diameter &&
               diameter < model_height &&
               aabb.get_center().y < model_mid_height;
    }

    static bool overlaps(const BoundingBox& a, const BoundingBox& b)
    {
        return a.min_corner.x <= b.max_corner.x + OVERLAP_EPSILON && b.min_corner.x <= a.max_corner.x + OVERLAP_EPSILON &&
               a.min_corner.y <= b.max_corner.y + OVERLAP_EPSILON && b.min_corner.y <= a.max_corner.y + OVERLAP_EPSILON &&
               a.min_corner.z <= b.max_corner.z + OVERLAP_EPSILON && b.min_corner.z <= a.max_corner.z + OVERLAP_EPSILON;
    }

    static bool contains(const BoundingBox& outer, const BoundingBox& inner)
    {
        return inner.min_corner.x >= outer.min_corner.x - OVERLAP_EPSILON && inner.max_corner.x <= outer.max_corner.x + OVERLAP_EPSILON &&
               inner.min_corner.y >= outer.min_corner.y - OVERLAP_EPSILON && inner.max_corner.y <= outer.max_corner.y + OVERLAP_EPSILON &&
               inner.min_corner.z >= outer.min_corner.z - OVERLAP_EPSILON && inner.max_corner.z <= outer.max_corner.z + OVERLAP_EPSILON;
    }

    struct PartCluster
    {
        BoundingBox aabb = BoundingBox::negative_infinity();
        std::vector<uint32> parts;
    };

    static std::vector<PartCluster> cluster_overlapping(const std::vector<VehicleImportPart>& parts, const std::vector<uint32>& indices)
    {
        std::vector<uint32> root(indices.size());
        std::iota(root.begin(), root.end(), 0);
        auto find_root = [&](uint32 i)
        {
            while (root[i] != i)
            {
                i = root[i] = root[root[i]];
            }
            return i;
        };

        for (uint32 i = 0; i < indices.size(); ++i)
        {
            for (uint32 j = i + 1; j < indices.size(); ++j)
            {
                if (overlaps(parts[indices[i]].aabb, parts[indices[j]].aabb))
                {
                    root[find_root(j)] = find_root(i);
                }
            }
        }

        std::vector<PartCluster> clusters;
        std::vector<int32> cluster_index(indices.size(), -1);
        for (uint32 i = 0; i < indices.size(); ++i)
        {
            const uint32 r = find_root(i);
            if (cluster_index[r] < 0)
            {
                cluster_index[r] = static_cast<int32>(clusters.size());
                clusters.emplace_back();
            }

            PartCluster& cluster = clusters[cluster_index[r]];
            cluster.aabb.grow(parts[indices[i]].aabb.min_corner);
            cluster.aabb.grow(parts[indices[i]].aabb.max_corner);
            cluster.parts.push_back(indices[i]);
        }
        return clusters;
    }

    static void attach_coaxial_wheel_parts(std::vector<VehicleImportPart>& parts)
    {
        std::vector<uint32> wheel_parts;
        for (uint32 i = 0; i < parts.size(); ++i)
        {
            if (parts[i].is_wheel)
            {
                wheel_parts.push_back(i);
            }
        }

        for (const PartCluster& cluster : cluster_overlapping(parts, wheel_parts))
        {
            const vec3 center = cluster.aabb.get_center();
            const vec3 extents = cluster.aabb.max_corner - cluster.aabb.min_corner;
            const float radius = std::max(extents.y, extents.z) * 0.5f;

            for (VehicleImportPart& part : parts)
            {
                if (part.is_wheel || part.is_door || part.is_steering_wheel)
                {
                    continue;
                }

                const vec3 part_center = part.aabb.get_center();
                const vec3 part_extents = part.aabb.max_corner - part.aabb.min_corner;
                const float part_diameter = std::max(part_extents.y, part_extents.z);

                const bool on_axis = std::abs(part_center.y - center.y) < 0.1f * radius && std::abs(part_center.z - center.z) < 0.1f * radius;
                const bool round = std::abs(part_extents.y - part_extents.z) < 0.1f * part_diameter;
                const bool beside = std::abs(part_center.x - center.x) < std::max(extents.x, part_extents.x);

                if (on_axis && round && beside && part_diameter > radius)
                {
                    part.is_wheel = true;
                }
            }
        }
    }

    static quat get_forward_rotation(VehicleForwardAxis axis)
    {
        const vec3 up(0.0f, 1.0f, 0.0f);
        switch (axis)
        {
            case VehicleForwardAxis::NEGATIVE_Z:
                return quat(up, M_PI);
            case VehicleForwardAxis::POSITIVE_X:
                return quat(up, -M_PI_OVER_2);
            case VehicleForwardAxis::NEGATIVE_X:
                return quat(up, M_PI_OVER_2);
            default:
                return quat::identity;
        }
    }

    // Parts that are not moved by wheels, doors or the steering wheel.
    static std::vector<bool> get_body_parts(const VehicleImportSetup& setup)
    {
        std::vector<bool> result(setup.parts.size(), true);
        auto exclude = [&](const std::vector<uint32>& parts)
        {
            for (uint32 part_index : parts)
            {
                result[part_index] = false;
            }
        };

        for (const VehicleImportWheel& wheel : setup.wheels)
        {
            exclude(wheel.parts);
            exclude(wheel.hub_parts);
        }
        for (const VehicleImportDoor& door : setup.doors)
        {
            exclude(door.parts);
        }
        exclude(setup.steering_wheel.parts);
        return result;
    }

    VehicleImportSetup VehicleImportUtils::analyze(const ModelAsset& model, const VehicleImportSettings& settings)
    {
        VehicleImportSetup setup;
        setup.model_rotation = get_forward_rotation(settings.forward_axis);
        setup.model_scale = settings.model_scale;

        if (setup.model_scale <= 0.0f)
        {
            BoundingBox raw_aabb = BoundingBox::negative_infinity();
            for (const MeshAsset& mesh : model.meshes)
            {
                for (const SubmeshAsset& submesh : mesh.submeshes)
                {
                    for (const vec3& position : submesh.positions)
                    {
                        raw_aabb.grow(position);
                    }
                }
            }
            const vec3 extents = raw_aabb.max_corner - raw_aabb.min_corner;
            setup.model_scale = std::max(extents.x, extents.z) > CENTIMETER_MODEL_LENGTH ? 0.01f : 1.0f;
        }

        const std::vector<vec3>& directions = get_hull_directions();

        BoundingBox model_aabb = BoundingBox::negative_infinity();

        for (uint32 mesh_index = 0; mesh_index < model.meshes.size(); ++mesh_index)
        {
            const MeshAsset& mesh = model.meshes[mesh_index];

            VehicleImportPart part;
            part.name = mesh.name;
            part.mesh_index = mesh_index;
            part.aabb = BoundingBox::negative_infinity();

            std::vector<float> support_distances(directions.size(), -FLT_MAX);
            part.support_points.resize(directions.size());

            bool has_vertices = false;
            for (const SubmeshAsset& submesh : mesh.submeshes)
            {
                for (const vec3& position : submesh.positions)
                {
                    const vec3 point = setup.model_rotation * (position * setup.model_scale);
                    part.aabb.grow(point);
                    has_vertices = true;

                    for (size_t d = 0; d < directions.size(); ++d)
                    {
                        const float distance = dot(point, directions[d]);
                        if (distance > support_distances[d])
                        {
                            support_distances[d] = distance;
                            part.support_points[d] = point;
                        }
                    }
                }
            }

            if (!has_vertices)
            {
                continue;
            }

            part.front_edge_x = vec2(FLT_MAX, -FLT_MAX);
            for (const SubmeshAsset& submesh : mesh.submeshes)
            {
                for (const vec3& position : submesh.positions)
                {
                    const vec3 point = setup.model_rotation * (position * setup.model_scale);
                    if (point.z >= part.aabb.max_corner.z - FRONT_EDGE_BAND)
                    {
                        part.front_edge_x.x = std::min(part.front_edge_x.x, point.x);
                        part.front_edge_x.y = std::max(part.front_edge_x.y, point.x);
                    }

                    part.moment_count += 1.0f;
                    part.moment_sum += vec2(point.y, point.z);
                    part.moment_sum2 += vec3(point.y * point.y, point.y * point.z, point.z * point.z);
                }
            }

            model_aabb.grow(part.aabb.min_corner);
            model_aabb.grow(part.aabb.max_corner);

            const std::string name = to_lower(part.name);
            part.is_door = is_door_name(name);
            part.is_steering_wheel = !part.is_door && is_steering_wheel_name(name);
            part.is_wheel = !part.is_door && !part.is_steering_wheel && is_wheel_name(name);
            setup.parts.push_back(part);
        }

        const bool has_named_wheels = std::any_of(setup.parts.begin(), setup.parts.end(), [](const VehicleImportPart& part)
            {
                return part.is_wheel;
            });
        if (!has_named_wheels)
        {
            for (VehicleImportPart& part : setup.parts)
            {
                part.is_wheel = !part.is_door && !part.is_steering_wheel && is_wheel_shape(part.aabb, model_aabb);
            }
        }

        attach_coaxial_wheel_parts(setup.parts);

        rebuild(setup, settings);
        return setup;
    }

    bool VehicleImportUtils::rebuild(VehicleImportSetup& setup, const VehicleImportSettings& settings)
    {
        setup.error.clear();
        setup.wheels.clear();
        setup.doors.clear();
        setup.steering_wheel = {};
        setup.model_offset = vec3::zero;

        auto fail = [&](std::string error)
        {
            setup.error = std::move(error);
            setup.wheels.clear();
            setup.doors.clear();
            setup.steering_wheel = {};
            return false;
        };

        std::vector<uint32> wheel_parts;
        for (uint32 i = 0; i < setup.parts.size(); ++i)
        {
            if (setup.parts[i].is_wheel)
            {
                wheel_parts.push_back(i);
            }
        }

        // Model space until the offset is applied at the end.
        std::vector<VehicleImportWheel>& wheels = setup.wheels;
        for (const PartCluster& wheel_cluster : cluster_overlapping(setup.parts, wheel_parts))
        {
            const vec3 extents = wheel_cluster.aabb.max_corner - wheel_cluster.aabb.min_corner;

            VehicleImportWheel wheel;
            wheel.center = wheel_cluster.aabb.get_center();
            wheel.radius = std::max(extents.y, extents.z) * 0.5f;
            wheel.width = extents.x;
            wheel.parts = wheel_cluster.parts;

            for (uint32 i = 0; i < setup.parts.size(); ++i)
            {
                const VehicleImportPart& part = setup.parts[i];
                if (!part.is_wheel && !part.is_door && !part.is_steering_wheel && contains(wheel_cluster.aabb, part.aabb))
                {
                    wheel.hub_parts.push_back(i);
                }
            }

            wheels.push_back(wheel);
        }

        std::sort(wheels.begin(), wheels.end(), [](const VehicleImportWheel& a, const VehicleImportWheel& b)
            {
                return a.center.z > b.center.z;
            });

        std::vector<std::vector<uint32>> axles;
        for (uint32 i = 0; i < wheels.size(); ++i)
        {
            if (!axles.empty())
            {
                const VehicleImportWheel& first = wheels[axles.back().front()];
                if (std::abs(first.center.z - wheels[i].center.z) < std::max(first.radius, wheels[i].radius))
                {
                    axles.back().push_back(i);
                    continue;
                }
            }
            axles.push_back({i});
        }

        if (axles.size() < 2)
        {
            return fail(std::format("Found {} wheel(s) on {} axle(s), need at least 2 axles.", wheels.size(), axles.size()));
        }

        for (const std::vector<uint32>& axle : axles)
        {
            if (axle.size() != 2 || std::abs(wheels[axle[0]].center.x - wheels[axle[1]].center.x) < wheels[axle[0]].width)
            {
                return fail(std::format("Axle at z = {:.2f} must have one left and one right wheel, found {}.", wheels[axle[0]].center.z, axle.size()));
            }
        }

        const uint32 nb_axles = static_cast<uint32>(axles.size());

        for (uint32 axle = 0; axle < nb_axles; ++axle)
        {
            const VehicleImportWheel& a = wheels[axles[axle][0]];
            const VehicleImportWheel& b = wheels[axles[axle][1]];
            const bool a_is_left = a.center.x > b.center.x;

            wheels[axles[axle][0]].wheel_id = axle * 2 + (a_is_left ? 1 : 0);
            wheels[axles[axle][1]].wheel_id = axle * 2 + (a_is_left ? 0 : 1);
        }

        float ground = wheels.front().center.y - wheels.front().radius;
        float center_x = 0.0f;
        float min_radius = wheels.front().radius;
        for (const VehicleImportWheel& wheel : wheels)
        {
            ground = std::min(ground, wheel.center.y - wheel.radius);
            center_x += wheel.center.x / wheels.size();
            min_radius = std::min(min_radius, wheel.radius);
        }
        const float center_z = (wheels[axles.front().front()].center.z + wheels[axles.back().front()].center.z) * 0.5f;

        setup.model_offset = -vec3(center_x, ground, center_z);

        float min_wheel_x = wheels.front().center.x;
        float max_wheel_x = min_wheel_x;
        float max_wheel_width = 0.0f;
        for (const VehicleImportWheel& wheel : wheels)
        {
            min_wheel_x = std::min(min_wheel_x, wheel.center.x);
            max_wheel_x = std::max(max_wheel_x, wheel.center.x);
            max_wheel_width = std::max(max_wheel_width, wheel.width);
        }
        const float front_z = wheels[axles.front().front()].center.z;
        const float rear_z = wheels[axles.back().front()].center.z;
        const float track = max_wheel_x - min_wheel_x;
        const float wheel_base = front_z - rear_z;
        const float margin_x = max_wheel_width + 0.5f * track;
        const float margin_z = min_radius + 0.5f * wheel_base;

        build_doors(setup);
        build_steering_wheel(setup);

        const std::vector<bool> is_body_part = get_body_parts(setup);

        const std::vector<vec3>& directions = get_hull_directions();
        std::vector<float> hull_distances(directions.size(), -FLT_MAX);
        std::vector<vec3> hull_points(directions.size());

        BoundingBox body_aabb = BoundingBox::negative_infinity();
        bool has_body = false;
        for (uint32 part_index = 0; part_index < setup.parts.size(); ++part_index)
        {
            const VehicleImportPart& part = setup.parts[part_index];
            if (!is_body_part[part_index])
            {
                continue;
            }

            const vec3 center = part.aabb.get_center();
            if (center.x < min_wheel_x - margin_x || center.x > max_wheel_x + margin_x || center.z < rear_z - margin_z || center.z > front_z + margin_z)
            {
                LOG_WARNING("Vehicle import: part '%s' is far from the wheels and is ignored for collision.", part.name.c_str());
                continue;
            }

            body_aabb.grow(part.aabb.min_corner + setup.model_offset);
            body_aabb.grow(part.aabb.max_corner + setup.model_offset);
            has_body = true;

            for (size_t d = 0; d < directions.size(); ++d)
            {
                const float distance = dot(part.support_points[d], directions[d]);
                if (distance > hull_distances[d])
                {
                    hull_distances[d] = distance;
                    hull_points[d] = part.support_points[d];
                }
            }
        }

        if (!has_body)
        {
            return fail("No body parts left after wheel selection.");
        }

        body_aabb.min_corner.y = std::max(body_aabb.min_corner.y, min_radius * 0.5f);
        if (body_aabb.max_corner.y <= body_aabb.min_corner.y)
        {
            return fail("Body is lower than the wheel clearance.");
        }

        const float mass_scale = settings.mass / 1500.0f;

        VehicleConfig& config = setup.config;
        config = VehicleConfig::make_default();
        config.axles.clear();

        config.chassis.mass = settings.mass;
        config.chassis.moi = vec3::zero;
        config.chassis.collision_center = body_aabb.get_center();
        config.chassis.collision_half_extents = (body_aabb.max_corner - body_aabb.min_corner) * 0.5f;
        config.chassis.center_of_mass = vec3(0.0f, body_aabb.min_corner.y + 0.3f * (body_aabb.max_corner.y - body_aabb.min_corner.y), 0.0f);

        config.chassis.collision_hull.clear();
        for (const vec3& hull_point : hull_points)
        {
            vec3 point = hull_point + setup.model_offset;
            point.y = std::max(point.y, body_aabb.min_corner.y);

            const bool is_duplicate = std::any_of(config.chassis.collision_hull.begin(), config.chassis.collision_hull.end(), [&](const vec3& other)
                {
                    return length(other - point) < OVERLAP_EPSILON;
                });
            if (!is_duplicate)
            {
                config.chassis.collision_hull.push_back(point);
            }
        }

        config.engine.peak_torque *= mass_scale;

        for (uint32 axle = 0; axle < nb_axles; ++axle)
        {
            const VehicleImportWheel& a = wheels[axles[axle][0]];
            const VehicleImportWheel& b = wheels[axles[axle][1]];

            const bool is_front = axle == 0;
            const bool is_rear = axle == nb_axles - 1;

            VehicleAxleConfig axle_config;
            axle_config.position = (a.center + b.center) * 0.5f + setup.model_offset;
            axle_config.track_width = std::abs(a.center.x - b.center.x);
            axle_config.wheel_radius = (a.radius + b.radius) * 0.5f;
            axle_config.wheel_width = (a.width + b.width) * 0.5f;
            axle_config.steered = is_front;

            switch (settings.drive_type)
            {
                case VehicleDriveType::FRONT_WHEELS:
                    axle_config.driven = is_front;
                    break;
                case VehicleDriveType::REAR_WHEELS:
                    axle_config.driven = !is_front;
                    break;
                default:
                    axle_config.driven = true;
                    break;
            }

            const float torque_scale = mass_scale * axle_config.wheel_radius / 0.34f;
            axle_config.brake_torque = (is_front ? 2000.0f : 1200.0f) * torque_scale;
            axle_config.handbrake_torque = is_rear ? 3000.0f * torque_scale : 0.0f;
            axle_config.anti_roll_ratio = is_front ? 0.6f : 0.3f;
            axle_config.tire_lateral_stiffness = is_front ? 20.0f : 28.0f;

            config.axles.push_back(axle_config);
        }

        for (VehicleImportWheel& wheel : wheels)
        {
            wheel.center += setup.model_offset;
        }
        return true;
    }

    void VehicleImportUtils::build_doors(VehicleImportSetup& setup)
    {
        std::vector<PartCluster> doors;
        for (uint32 i = 0; i < setup.parts.size(); ++i)
        {
            const VehicleImportPart& part = setup.parts[i];
            if (!part.is_door)
            {
                continue;
            }

            auto same_object = std::find_if(doors.begin(), doors.end(), [&](const PartCluster& door)
                {
                    return setup.parts[door.parts.front()].name == part.name;
                });
            if (same_object == doors.end())
            {
                doors.emplace_back();
                same_object = doors.end() - 1;
            }
            same_object->aabb.grow(part.aabb.min_corner);
            same_object->aabb.grow(part.aabb.max_corner);
            same_object->parts.push_back(i);
        }

        for (size_t i = 0; i < doors.size(); ++i)
        {
            for (size_t j = 0; j < doors.size(); ++j)
            {
                if (i != j && !doors[i].parts.empty() && !doors[j].parts.empty() && contains(doors[j].aabb, doors[i].aabb))
                {
                    doors[j].parts.insert(doors[j].parts.end(), doors[i].parts.begin(), doors[i].parts.end());
                    doors[i].parts.clear();
                }
            }
        }
        std::erase_if(doors, [](const PartCluster& door)
            {
                return door.parts.empty();
            });

        for (uint32 i = 0; i < setup.parts.size(); ++i)
        {
            const VehicleImportPart& part = setup.parts[i];
            if (part.is_wheel || part.is_door || part.is_steering_wheel)
            {
                continue;
            }

            for (PartCluster& door : doors)
            {
                if (contains(door.aabb, part.aabb))
                {
                    door.parts.push_back(i);
                    break;
                }
            }
        }

        std::sort(doors.begin(), doors.end(), [](const PartCluster& a, const PartCluster& b)
            {
                return a.aabb.get_center().z > b.aabb.get_center().z;
            });

        uint32 nb_left = 0;
        uint32 nb_right = 0;
        for (const PartCluster& door : doors)
        {
            const bool left = door.aabb.get_center().x > 0.0f;
            const uint32 index = left ? nb_left++ : nb_right++;

            const VehicleImportPart* front_part = &setup.parts[door.parts.front()];
            for (uint32 part_index : door.parts)
            {
                if (setup.parts[part_index].aabb.max_corner.z > front_part->aabb.max_corner.z)
                {
                    front_part = &setup.parts[part_index];
                }
            }

            VehicleImportDoor result;
            result.label = std::format("{}{}", index == 0 ? "F" : (index == 1 ? "R" : std::to_string(index).c_str()), left ? "L" : "R");
            result.hinge = vec3(left ? front_part->front_edge_x.y : front_part->front_edge_x.x, door.aabb.get_center().y, front_part->aabb.max_corner.z) + setup.model_offset;
            result.max_angle = left ? -DOOR_OPEN_ANGLE : DOOR_OPEN_ANGLE;
            result.collision_hull = merge_support_points(setup.parts, door.parts, setup.model_offset - result.hinge);
            result.parts = door.parts;
            setup.doors.push_back(result);
        }
    }

    void VehicleImportUtils::build_steering_wheel(VehicleImportSetup& setup)
    {
        BoundingBox aabb = BoundingBox::negative_infinity();
        float count = 0.0f;
        vec2 sum = vec2(0.0f);
        vec3 sum2 = vec3::zero;

        for (uint32 i = 0; i < setup.parts.size(); ++i)
        {
            const VehicleImportPart& part = setup.parts[i];
            if (!part.is_steering_wheel)
            {
                continue;
            }

            aabb.grow(part.aabb.min_corner);
            aabb.grow(part.aabb.max_corner);
            count += part.moment_count;
            sum += part.moment_sum;
            sum2 += part.moment_sum2;
            setup.steering_wheel.parts.push_back(i);
        }

        if (count <= 0.0f)
        {
            return;
        }

        const vec2 mean = sum / count;
        const float yy = sum2.x / count - mean.x * mean.x;
        const float yz = sum2.y / count - mean.x * mean.y;
        const float zz = sum2.z / count - mean.y * mean.y;

        const float trace = yy + zz;
        const float det = yy * zz - yz * yz;
        const float smallest = trace * 0.5f - sqrt(std::max(0.0f, trace * trace * 0.25f - det));

        vec2 axis = std::abs(yz) > 1e-8f ? vec2(yz, smallest - yy) : (yy < zz ? vec2(1.0f, 0.0f) : vec2(0.0f, 1.0f));
        axis = normalize(axis);
        if (axis.y > 0.0f)
        {
            axis = -axis;
        }

        setup.steering_wheel.center = aabb.get_center() + setup.model_offset;
        setup.steering_wheel.axis = vec3(0.0f, axis.x, axis.y);
    }

    // Saves the parts as one mesh with vertices relative to `origin` (vehicle space).
    static ref<MultiMesh> create_part_mesh(const ModelAsset& source_model,
        uint32 flags,
        const VehicleImportSetup& setup,
        const std::vector<uint32>& parts,
        const vec3& origin,
        const fs::path& path_without_extension)
    {
        ModelAsset model;
        model.flags = source_model.flags;
        model.materials = source_model.materials;

        const vec3 offset = setup.model_offset - origin;
        for (uint32 part_index : parts)
        {
            MeshAsset mesh = source_model.meshes[setup.parts[part_index].mesh_index];
            for (SubmeshAsset& submesh : mesh.submeshes)
            {
                for (vec3& position : submesh.positions)
                {
                    position = setup.model_rotation * (position * setup.model_scale) + offset;
                }
                for (vec3& normal : submesh.normals)
                {
                    normal = setup.model_rotation * normal;
                }
                for (vec3& tangent : submesh.tangents)
                {
                    tangent = setup.model_rotation * tangent;
                }
                submesh.skin.clear();
            }
            model.meshes.push_back(std::move(mesh));
        }

        ref<MultiMesh> result = make_ref<MultiMesh>();
        result->flags = flags;
        result->update_mesh(model);
        result->source_path = fs::path(path_without_extension).concat(MultiMesh::get_asset_type_impl()).lexically_normal().make_preferred();

        GameAssetsProvider provider;
        provider.save_game_asset_to_file_async<MultiMesh>(path_without_extension, result.get()).wait_for_completion();

        return result;
    }

    static Entity create_child(World* world, Entity root, const std::string& name, const vec3& position, const ref<MultiMesh>& mesh)
    {
        Entity entity = world->create_entity(name.c_str());
        entity.set_parent(root.get_handle());
        entity.get_component<TransformComponent>()->set_local_transform(trs(position, quat::identity, vec3(1.0f)));
        entity.add_component<MeshComponent>(mesh);
        return entity;
    }

    Entity VehicleImportUtils::create_vehicle(World* world,
        const ref<MultiMesh>& source,
        const VehicleImportSetup& setup,
        const fs::path& output_directory,
        const std::string& name)
    {
        if (world == nullptr || source == nullptr || !source->model_asset.has_value() || !setup.error.empty() || setup.wheels.empty())
        {
            LOG_ERROR("Vehicle import: invalid setup.");
            return Entity::null;
        }

        const ModelAsset& model = *source->model_asset;
        const uint32 flags = source->flags & ~MESH_CREATION_FLAGS_WITH_SKIN;

        const std::vector<bool> is_body_part = get_body_parts(setup);
        std::vector<uint32> body_parts;
        for (uint32 i = 0; i < setup.parts.size(); ++i)
        {
            if (is_body_part[i])
            {
                body_parts.push_back(i);
            }
        }

        Entity root = world->create_entity(name.c_str());
        root.add_component<MeshComponent>(create_part_mesh(model, flags, setup, body_parts, vec3::zero, output_directory / (name + "_body")));
        root.add_component<W4VehicleComponent>()->config = setup.config;

        const uint32 nb_axles = static_cast<uint32>(setup.config.axles.size());

        for (const VehicleImportWheel& wheel : setup.wheels)
        {
            const std::string label = get_wheel_label(wheel.wheel_id, nb_axles);

            ref<MultiMesh> wheel_mesh = create_part_mesh(model, flags, setup, wheel.parts, wheel.center, output_directory / std::format("{}_wheel_{}", name, label));
            create_child(world, root, std::format("wheel_{}", label), wheel.center, wheel_mesh).add_component<VehicleWheelComponent>(wheel.wheel_id);

            if (!wheel.hub_parts.empty())
            {
                ref<MultiMesh> hub_mesh = create_part_mesh(model, flags, setup, wheel.hub_parts, wheel.center, output_directory / std::format("{}_hub_{}", name, label));
                create_child(world, root, std::format("hub_{}", label), wheel.center, hub_mesh).add_component<VehicleWheelComponent>(wheel.wheel_id)->should_spin = false;
            }
        }

        for (const VehicleImportDoor& door : setup.doors)
        {
            ref<MultiMesh> door_mesh = create_part_mesh(model, flags, setup, door.parts, door.hinge, output_directory / std::format("{}_door_{}", name, door.label));
            VehicleDoorComponent* door_component = create_child(world, root, std::format("door_{}", door.label), door.hinge, door_mesh).add_component<VehicleDoorComponent>();
            door_component->max_angle = door.max_angle;
            door_component->collision_hull = door.collision_hull;
        }

        if (!setup.steering_wheel.parts.empty())
        {
            const VehicleImportSteeringWheel& steering = setup.steering_wheel;
            ref<MultiMesh> steering_mesh = create_part_mesh(model, flags, setup, steering.parts, steering.center, output_directory / (name + "_steering_wheel"));
            create_child(world, root, "steering_wheel", steering.center, steering_mesh).add_component<VehicleSteeringWheelComponent>()->axis = steering.axis;
        }

        const fs::path prefab_path = output_directory / (name + ".eprefab");
        if (!PrefabSerializer::save_to_file({root}, prefab_path))
        {
            LOG_ERROR("Vehicle import: failed to save prefab '%s'.", prefab_path.string().c_str());
        }

        return root;
    }

    std::string VehicleImportUtils::get_wheel_label(uint32 wheel_id, uint32 nb_axles)
    {
        const uint32 axle = wheel_id / 2;
        const char* side = (wheel_id % 2) == 1 ? "L" : "R";

        if (axle == 0)
        {
            return std::format("F{}", side);
        }
        if (axle + 1 == nb_axles)
        {
            return std::format("R{}", side);
        }
        return std::format("M{}{}", axle, side);
    }
}
