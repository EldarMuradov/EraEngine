#pragma once

#include "physics_api.h"

#include "physics/vehicles/vehicle_config.h"

#include "core/math.h"
#include "core/bounding_volumes.h"

#include "asset/model_asset.h"

#include "ecs/entity.h"

#include <string>
#include <vector>

namespace era_engine
{
    class MultiMesh;
    class World;
}

namespace era_engine::physics
{
    enum class VehicleForwardAxis : uint8
    {
        POSITIVE_Z,
        NEGATIVE_Z,
        POSITIVE_X,
        NEGATIVE_X
    };

    enum class VehicleDriveType : uint8
    {
        ALL_WHEELS,
        FRONT_WHEELS,
        REAR_WHEELS
    };

    struct VehicleImportSettings
    {
        VehicleForwardAxis forward_axis = VehicleForwardAxis::POSITIVE_Z;
        VehicleDriveType drive_type = VehicleDriveType::ALL_WHEELS;
        float mass = 1500.0f;

        // Model units to meters, 0 = auto.
        float model_scale = 0.0f;
    };

    struct VehicleImportPart
    {
        std::string name;
        uint32 mesh_index = 0;
        BoundingBox aabb;
        bool is_wheel = false;
        bool is_door = false;
        bool is_steering_wheel = false;

        // X range of the vertices at the front-most edge.
        vec2 front_edge_x = vec2(0.0f);

        // Vertex count and raw Y/Z moments: sum(y), sum(z), sum(y*y), sum(y*z), sum(z*z).
        float moment_count = 0.0f;
        vec2 moment_sum = vec2(0.0f);
        vec3 moment_sum2 = vec3::zero;

        // Extreme vertices along fixed directions, rotated and scaled model space.
        std::vector<vec3> support_points;
    };

    struct VehicleImportWheel
    {
        uint32 wheel_id = 0;
        vec3 center = vec3::zero;
        float radius = 0.0f;
        float width = 0.0f;
        std::vector<uint32> parts;
        std::vector<uint32> hub_parts; // Inside the wheel, steer without spinning (brake calipers).
    };

    struct VehicleImportDoor
    {
        std::string label;
        vec3 hinge = vec3::zero;          // Vehicle space, vertical axis.
        float max_angle = 0.0f;           // Degrees, sign opens the door outwards.
        std::vector<vec3> collision_hull; // Relative to the hinge.
        std::vector<uint32> parts;
    };

    struct VehicleImportSteeringWheel
    {
        vec3 center = vec3::zero;            // Vehicle space.
        vec3 axis = vec3(0.0f, 0.0f, -1.0f); // Column axis, points to the driver.
        std::vector<uint32> parts;
    };

    struct VehicleImportSetup
    {
        std::vector<VehicleImportPart> parts;
        std::vector<VehicleImportWheel> wheels;
        std::vector<VehicleImportDoor> doors;
        VehicleImportSteeringWheel steering_wheel;
        VehicleConfig config;

        float model_scale = 1.0f;
        quat model_rotation = quat::identity;
        vec3 model_offset = vec3::zero;

        std::string error;
    };

    class ERA_PHYSICS_API VehicleImportUtils
    {
    public:
        VehicleImportUtils() = delete;

        static VehicleImportSetup analyze(const ModelAsset& model, const VehicleImportSettings& settings);

        static bool rebuild(VehicleImportSetup& setup, const VehicleImportSettings& settings);

        static Entity create_vehicle(World* world,
            const ref<MultiMesh>& source,
            const VehicleImportSetup& setup,
            const fs::path& output_directory,
            const std::string& name);

        static std::string get_wheel_label(uint32 wheel_id, uint32 nb_axles);

    private:
        static void build_doors(VehicleImportSetup& setup);
        static void build_steering_wheel(VehicleImportSetup& setup);
    };
}
