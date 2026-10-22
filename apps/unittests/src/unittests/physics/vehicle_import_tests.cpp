#include <gtest/gtest.h>

#include <physics/vehicles/vehicle_import_utils.h>

#include <format>

namespace
{
    using namespace era_engine;
    using namespace era_engine::physics;

    void add_box(ModelAsset& model, const std::string& name, const vec3& center, const vec3& half_extents)
    {
        SubmeshAsset submesh;
        for (uint32 i = 0; i < 8; ++i)
        {
            const vec3 sign((i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f, (i & 4) ? 1.0f : -1.0f);
            submesh.positions.push_back(center + sign * half_extents);
        }

        MeshAsset mesh;
        mesh.name = name;
        mesh.submeshes.push_back(submesh);
        model.meshes.push_back(mesh);
    }

    // Front at +Z, wheels at +-0.8 / +-1.4, radius 0.34, ground at y = 0.
    ModelAsset make_car(const std::vector<std::string>& wheel_names, const vec3& offset = vec3::zero)
    {
        ModelAsset model;
        add_box(model, "Body", vec3(0.0f, 0.8f, 0.0f) + offset, vec3(0.9f, 0.6f, 2.3f));

        const vec3 wheel_centers[] = {vec3(0.8f, 0.34f, 1.4f), vec3(-0.8f, 0.34f, 1.4f), vec3(0.8f, 0.34f, -1.4f), vec3(-0.8f, 0.34f, -1.4f)};
        for (size_t i = 0; i < wheel_names.size(); ++i)
        {
            add_box(model, wheel_names[i], wheel_centers[i] + offset, vec3(0.12f, 0.34f, 0.34f));
        }
        return model;
    }

    const VehicleImportWheel* find_wheel(const VehicleImportSetup& setup, uint32 wheel_id)
    {
        for (const VehicleImportWheel& wheel : setup.wheels)
        {
            if (wheel.wheel_id == wheel_id)
            {
                return &wheel;
            }
        }
        return nullptr;
    }
}

TEST(VehicleImportTest, DetectsNamedWheelsAndBuildsConfig)
{
    const VehicleImportSetup setup = VehicleImportUtils::analyze(make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"}), {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    ASSERT_EQ(setup.wheels.size(), 4u);
    ASSERT_EQ(setup.config.axles.size(), 2u);

    const VehicleAxleConfig& front = setup.config.axles[0];
    EXPECT_TRUE(front.steered);
    EXPECT_FALSE(setup.config.axles[1].steered);
    EXPECT_NEAR(front.position.z, 1.4f, 1e-4f);
    EXPECT_NEAR(front.position.y, 0.34f, 1e-4f);
    EXPECT_NEAR(front.track_width, 1.6f, 1e-4f);
    EXPECT_NEAR(front.wheel_radius, 0.34f, 1e-4f);
    EXPECT_NEAR(front.wheel_width, 0.24f, 1e-4f);

    // Front left is +X.
    const VehicleImportWheel* front_left = find_wheel(setup, 1);
    ASSERT_NE(front_left, nullptr);
    EXPECT_GT(front_left->center.x, 0.0f);
    EXPECT_GT(front_left->center.z, 0.0f);
    EXPECT_EQ(setup.parts[front_left->parts[0]].name, "Wheel_FL");

    EXPECT_GE(setup.config.chassis.collision_center.y - setup.config.chassis.collision_half_extents.y, 0.17f - 1e-4f);
}

TEST(VehicleImportTest, FallsBackToShapeDetection)
{
    const VehicleImportSetup setup = VehicleImportUtils::analyze(make_car({"Object001", "Object002", "Object003", "Object004"}), {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    EXPECT_EQ(setup.wheels.size(), 4u);
    EXPECT_FALSE(setup.parts[0].is_wheel);
}

TEST(VehicleImportTest, MergesOverlappingWheelParts)
{
    ModelAsset model = make_car({"Tire_FL", "Tire_FR", "Tire_RL", "Tire_RR"});
    add_box(model, "Rim_FL", vec3(0.8f, 0.34f, 1.4f), vec3(0.1f, 0.25f, 0.25f));

    const VehicleImportSetup setup = VehicleImportUtils::analyze(model, {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    ASSERT_EQ(setup.wheels.size(), 4u);
    EXPECT_EQ(find_wheel(setup, 1)->parts.size(), 2u);
}

TEST(VehicleImportTest, RecentersModelOnGround)
{
    const VehicleImportSetup setup = VehicleImportUtils::analyze(make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"}, vec3(3.0f, 2.0f, 5.0f)), {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    EXPECT_TRUE(fuzzy_equals(setup.model_offset, vec3(-3.0f, -2.0f, -5.0f)));
    EXPECT_NEAR(setup.config.axles[0].position.y, 0.34f, 1e-4f);
    EXPECT_NEAR(setup.config.axles[1].position.z, -1.4f, 1e-4f);
}

TEST(VehicleImportTest, ForwardAxisRotatesModel)
{
    VehicleImportSettings settings;
    settings.forward_axis = VehicleForwardAxis::NEGATIVE_Z;

    // Model front is -Z: the original rear wheels become the steered front axle.
    const VehicleImportSetup setup = VehicleImportUtils::analyze(make_car({"Wheel_RR", "Wheel_RL", "Wheel_FR", "Wheel_FL"}), settings);

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    const VehicleImportWheel* front_left = find_wheel(setup, 1);
    ASSERT_NE(front_left, nullptr);
    EXPECT_EQ(setup.parts[front_left->parts[0]].name, "Wheel_FL");
    EXPECT_GT(front_left->center.z, 0.0f);
}

TEST(VehicleImportTest, ReportsIncompleteAxle)
{
    const VehicleImportSetup setup = VehicleImportUtils::analyze(make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL"}), {});

    EXPECT_FALSE(setup.error.empty());
}

TEST(VehicleImportTest, DriveTypeSelectsDrivenAxles)
{
    VehicleImportSettings settings;
    settings.drive_type = VehicleDriveType::REAR_WHEELS;

    const VehicleImportSetup setup = VehicleImportUtils::analyze(make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"}), settings);

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    EXPECT_FALSE(setup.config.axles[0].driven);
    EXPECT_TRUE(setup.config.axles[1].driven);
    EXPECT_GT(setup.config.axles[1].handbrake_torque, 0.0f);
}

TEST(VehicleImportTest, StrayPartsDoNotStretchCollisionBox)
{
    ModelAsset model = make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"});
    add_box(model, "Lettering", vec3(-5.0f, 0.7f, 0.9f), vec3(0.01f, 0.01f, 0.03f));

    const VehicleImportSetup setup = VehicleImportUtils::analyze(model, {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    EXPECT_NEAR(setup.config.chassis.collision_center.x, 0.0f, 1e-4f);
    EXPECT_NEAR(setup.config.chassis.collision_half_extents.x, 0.9f, 1e-4f);
}

TEST(VehicleImportTest, DetectsCentimeterModels)
{
    ModelAsset model = make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"});
    for (MeshAsset& mesh : model.meshes)
    {
        for (vec3& position : mesh.submeshes[0].positions)
        {
            position *= 100.0f;
        }
    }

    const VehicleImportSetup setup = VehicleImportUtils::analyze(model, {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    EXPECT_FLOAT_EQ(setup.model_scale, 0.01f);
    EXPECT_NEAR(setup.config.axles[0].wheel_radius, 0.34f, 1e-3f);
    EXPECT_NEAR(setup.config.axles[0].position.z, 1.4f, 1e-3f);
}

TEST(VehicleImportTest, BuildsCollisionHullAroundBody)
{
    const VehicleImportSetup setup = VehicleImportUtils::analyze(make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"}), {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;

    const std::vector<vec3>& hull = setup.config.chassis.collision_hull;
    ASSERT_GE(hull.size(), 4u);
    EXPECT_LE(hull.size(), 96u);

    // Body box corners are (+-0.9, 0.2..1.4, +-2.3. Every corner must be a hull point.
    for (uint32 i = 0; i < 8; ++i)
    {
        const vec3 corner((i & 1) ? 0.9f : -0.9f, (i & 2) ? 1.4f : 0.2f, (i & 4) ? 2.3f : -2.3f);
        const bool found = std::any_of(hull.begin(), hull.end(), [&](const vec3& point)
            {
                return length(point - corner) < 1e-3f;
            });
        EXPECT_TRUE(found) << corner.x << " " << corner.y << " " << corner.z;
    }
}

TEST(VehicleImportTest, AttachesCoaxialTiresAndCalipersAsHubs)
{
    ModelAsset model = make_car({"Rim_FL", "Rim_FR", "Rim_RL", "Rim_RR"});
    const vec3 wheel_centers[] = {vec3(0.8f, 0.34f, 1.4f), vec3(-0.8f, 0.34f, 1.4f), vec3(0.8f, 0.34f, -1.4f), vec3(-0.8f, 0.34f, -1.4f)};
    for (uint32 i = 0; i < 4; ++i)
    {
        add_box(model, std::format("polySurface{}", i), wheel_centers[i], vec3(0.12f, 0.4f, 0.4f));
    }
    add_box(model, "Caliper", vec3(0.8f, 0.34f, 1.2f), vec3(0.05f, 0.1f, 0.05f));

    const VehicleImportSetup setup = VehicleImportUtils::analyze(model, {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    ASSERT_EQ(setup.wheels.size(), 4u);
    EXPECT_NEAR(setup.config.axles[0].wheel_radius, 0.4f, 1e-4f);
    for (const VehicleImportWheel& wheel : setup.wheels)
    {
        EXPECT_EQ(wheel.parts.size(), 2u);
    }
    EXPECT_FALSE(setup.parts.back().is_wheel);

    const uint32 caliper_index = static_cast<uint32>(setup.parts.size() - 1);
    for (const VehicleImportWheel& wheel : setup.wheels)
    {
        EXPECT_EQ(wheel.hub_parts, wheel.wheel_id == 1 ? std::vector<uint32>{caliper_index} : std::vector<uint32>{});
    }
    const VehicleImportWheel* front_left = find_wheel(setup, 1);
    ASSERT_NE(front_left, nullptr);
    EXPECT_TRUE(fuzzy_equals(front_left->center, vec3(0.8f, 0.4f, 1.4f)));
}

TEST(VehicleImportTest, BuildsDoorsWithHingesAndContainedParts)
{
    ModelAsset model = make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"});
    add_box(model, "Door_Left", vec3(0.85f, 0.8f, 0.3f), vec3(0.05f, 0.45f, 0.55f));
    add_box(model, "Door_Left_Handle", vec3(0.88f, 0.9f, 0.0f), vec3(0.01f, 0.02f, 0.05f));
    add_box(model, "Glass", vec3(0.85f, 1.1f, 0.3f), vec3(0.02f, 0.1f, 0.4f));
    add_box(model, "Door_Right", vec3(-0.85f, 0.8f, 0.3f), vec3(0.05f, 0.45f, 0.55f));
    add_box(model, "Door_Frame_Left", vec3(0.85f, 0.8f, 0.9f), vec3(0.05f, 0.45f, 0.02f));

    const VehicleImportSetup setup = VehicleImportUtils::analyze(model, {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    ASSERT_EQ(setup.doors.size(), 2u);

    const auto left = std::find_if(setup.doors.begin(), setup.doors.end(), [](const VehicleImportDoor& door)
        {
            return door.label == "FL";
        });
    const auto right = std::find_if(setup.doors.begin(), setup.doors.end(), [](const VehicleImportDoor& door)
        {
            return door.label == "FR";
        });
    ASSERT_NE(left, setup.doors.end());
    ASSERT_NE(right, setup.doors.end());

    EXPECT_EQ(left->parts.size(), 3u);
    EXPECT_EQ(right->parts.size(), 1u);
    EXPECT_TRUE(fuzzy_equals(left->hinge, vec3(0.9f, 0.8f, 0.85f)));
    EXPECT_LT(left->max_angle, 0.0f);
    EXPECT_GT(right->max_angle, 0.0f);
}

TEST(VehicleImportTest, DoorHingeIgnoresMirrorAndDoorHasHull)
{
    ModelAsset model = make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"});
    add_box(model, "Door_Left", vec3(0.85f, 0.8f, 0.3f), vec3(0.05f, 0.45f, 0.55f));
    add_box(model, "Door_Left", vec3(1.05f, 1.0f, 0.5f), vec3(0.1f, 0.05f, 0.08f));

    const VehicleImportSetup setup = VehicleImportUtils::analyze(model, {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    ASSERT_EQ(setup.doors.size(), 1u);

    const VehicleImportDoor& door = setup.doors.front();
    EXPECT_NEAR(door.hinge.x, 0.9f, 1e-4f);
    EXPECT_NEAR(door.hinge.z, 0.85f, 1e-4f);

    ASSERT_GE(door.collision_hull.size(), 8u);
    for (const vec3& point : door.collision_hull)
    {
        EXPECT_LE(point.z, 1e-4f);
    }
}

TEST(VehicleImportTest, FindsSteeringWheelAxis)
{
    ModelAsset model = make_car({"Wheel_FL", "Wheel_FR", "Wheel_RL", "Wheel_RR"});
    add_box(model, "Steering_Wheel", vec3(0.0f), vec3(0.18f, 0.18f, 0.02f));

    const quat tilt(vec3(1.0f, 0.0f, 0.0f), deg2rad(20.0f));
    for (vec3& position : model.meshes.back().submeshes[0].positions)
    {
        position = tilt * position + vec3(0.4f, 0.95f, 0.5f);
    }

    const VehicleImportSetup setup = VehicleImportUtils::analyze(model, {});

    ASSERT_TRUE(setup.error.empty()) << setup.error;
    ASSERT_EQ(setup.steering_wheel.parts.size(), 1u);
    EXPECT_FALSE(setup.parts.back().is_wheel);

    const vec3 expected_axis = tilt * vec3(0.0f, 0.0f, -1.0f);
    EXPECT_NEAR(dot(setup.steering_wheel.axis, expected_axis), 1.0f, 1e-3f);
    EXPECT_NEAR(setup.steering_wheel.center.x, 0.4f, 1e-4f);
}
