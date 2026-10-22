#include <gtest/gtest.h>

#include <physics_module.h>

#include <physics/body_component.h>
#include <physics/collision_types.h>
#include <physics/collisions_holder_root_component.h>
#include <physics/scene_queries.h>
#include <physics/shape_component.h>
#include <physics/core/physics.h>
#include <physics/core/physics_types.h>
#include <physics/core/physics_utils.h>
#include <physics/aggregate_holder_component.h>
#include <physics/vehicles/w4_vehicle_component.h>

#include <ecs/world.h>
#include <ecs/world_system_scheduler.h>
#include <ecs/base_components/transform_component.h>

#include <terrain/terrain.h>

#include <rttr/type>

#include <vector>

namespace
{
    using namespace era_engine;
    using namespace era_engine::physics;

    constexpr float fixed_dt = 1.0f / 60.0f;

    constexpr uint32 to_bits(CollisionType type)
    {
        return static_cast<uint32>(type);
    }

    class PhysicsTestEnvironment : public ::testing::Environment
    {
    public:
        static bool ensure_initialized()
        {
            if (module == nullptr)
            {
                World* gameplay_world = new World(World::GAMEPLAY_WORLD_NAME);
                gameplay_world->init();

                module = new PhysicsModule();
                initialized = module->initialize(nullptr);

                delete gameplay_world;
            }

            return initialized && PhysicsEngine::get_physics_core() != nullptr;
        }

        void TearDown() override
        {
            if (module != nullptr)
            {
                module->terminate();
                delete module;
                module = nullptr;
            }
        }

    private:
        static inline PhysicsModule* module = nullptr;
        static inline bool initialized = false;
    };

    ::testing::Environment* const physics_environment = ::testing::AddGlobalTestEnvironment(new PhysicsTestEnvironment());

    class PhysicsSimulationTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            ASSERT_TRUE(PhysicsTestEnvironment::ensure_initialized());

            world = new World("UnitTestPhysicsWorld");
            world->init();
            world->add_tag("physics");

            CollisionsHolderRootComponent* collisions = world->add_root_component<CollisionsHolderRootComponent>();
            collisions->set_collision_filter(to_bits(CollisionType::COLLISION_0), to_bits(CollisionType::COLLISION_0 | CollisionType::TERRAIN), true);

            scheduler = world->get_system_scheduler();
            scheduler->set_fixed_update_rate(60.0);

            std::vector<rttr::type> types = get_system_types();
            for (const rttr::type& type : types)
            {
                ASSERT_TRUE(type.is_valid());
            }

            scheduler->initialize_systems(rttr::array_range<rttr::type>(types.data(), types.size()));
            scheduler->initialize_all_systems();
        }

        void TearDown() override
        {
            delete world;
        }

        virtual std::vector<rttr::type> get_system_types() const
        {
            return {rttr::type::get_by_name("PhysicsSystem"), rttr::type::get_by_name("ShapeSystem")};
        }

        void step(uint32 count)
        {
            for (uint32 i = 0; i < count; ++i)
            {
                scheduler->update_fixed(fixed_dt);
            }
        }

        Entity create_sphere(const vec3& position, float radius = 0.5f, CollisionType collision_type = CollisionType::COLLISION_0)
        {
            Entity entity = world->create_entity("Sphere");
            entity.get_component<TransformComponent>()->set_world_position(position);

            SphereShapeComponent* shape = entity.add_component<SphereShapeComponent>();
            shape->radius = radius;
            shape->collision_type = collision_type;

            DynamicBodyComponent* body = entity.add_component<DynamicBodyComponent>();
            body->simulated = true;

            return entity;
        }

        Entity create_ground(CollisionType collision_type = CollisionType::TERRAIN)
        {
            Entity entity = world->create_entity("Ground");
            entity.get_component<TransformComponent>()->set_world_position(vec3(0.0f, -0.5f, 0.0f));

            BoxShapeComponent* shape = entity.add_component<BoxShapeComponent>();
            shape->half_extents = vec3(10.0f, 0.5f, 10.0f);
            shape->collision_type = collision_type;

            StaticBodyComponent* body = entity.add_component<StaticBodyComponent>();
            body->simulated = true;

            return entity;
        }

        vec3 position(Entity& entity)
        {
            return entity.get_component<TransformComponent>()->get_world_position();
        }

        DynamicBodyComponent* body(Entity& entity)
        {
            return entity.get_component<DynamicBodyComponent>();
        }

        World* world = nullptr;
        WorldSystemScheduler* scheduler = nullptr;
    };
}

TEST_F(PhysicsSimulationTest, BodiesAreCreatedOnNextFixedStep)
{
    Entity sphere = create_sphere(vec3(0.0f, 10.0f, 0.0f));
    Entity ground = create_ground();

    EXPECT_EQ(body(sphere)->get_rigid_actor(), nullptr);
    EXPECT_EQ(ground.get_component<StaticBodyComponent>()->get_rigid_actor(), nullptr);

    step(1);

    ASSERT_NE(body(sphere)->get_rigid_dynamic(), nullptr);
    ASSERT_NE(ground.get_component<StaticBodyComponent>()->get_rigid_static(), nullptr);
    EXPECT_NE(sphere.get_component<SphereShapeComponent>()->get_shape(), nullptr);
    EXPECT_EQ(body(sphere)->get_rigid_actor()->userData, static_cast<void*>(body(sphere)));
}

TEST_F(PhysicsSimulationTest, DynamicBodyFallsUnderGravity)
{
    Entity sphere = create_sphere(vec3(1.0f, 10.0f, -2.0f));

    step(60);

    const vec3 result = position(sphere);
    EXPECT_NEAR(result.y, 10.0f - 0.5f * 9.81f, 0.15f);
    EXPECT_NEAR(result.x, 1.0f, 1e-3f);
    EXPECT_NEAR(result.z, -2.0f, 1e-3f);
    EXPECT_NEAR(body(sphere)->linear_velocity.get().y, -9.81f, 0.2f);
}

TEST_F(PhysicsSimulationTest, NotSimulatedBodyStaysInPlace)
{
    Entity sphere = create_sphere(vec3(0.0f, 10.0f, 0.0f));
    body(sphere)->simulated = false;

    step(30);

    EXPECT_TRUE(fuzzy_equals(position(sphere), vec3(0.0f, 10.0f, 0.0f)));
}

TEST_F(PhysicsSimulationTest, GravityCanBeDisabled)
{
    Entity sphere = create_sphere(vec3(0.0f, 10.0f, 0.0f));
    body(sphere)->use_gravity = false;

    step(30);

    EXPECT_TRUE(fuzzy_equals(position(sphere), vec3(0.0f, 10.0f, 0.0f)));
}

TEST_F(PhysicsSimulationTest, KinematicBodyIgnoresGravity)
{
    Entity sphere = create_sphere(vec3(0.0f, 10.0f, 0.0f));
    body(sphere)->kinematic = true;

    step(30);

    EXPECT_TRUE(fuzzy_equals(position(sphere), vec3(0.0f, 10.0f, 0.0f)));
}

TEST_F(PhysicsSimulationTest, InitialVelocityIsApplied)
{
    Entity sphere = create_sphere(vec3(0.0f, 10.0f, 0.0f));
    body(sphere)->use_gravity = false;
    body(sphere)->linear_velocity = vec3(2.0f, 0.0f, 0.0f);

    step(60);

    EXPECT_NEAR(position(sphere).x, 2.0f, 0.05f);
    EXPECT_NEAR(position(sphere).y, 10.0f, 1e-3f);
    EXPECT_NEAR(body(sphere)->linear_velocity.get().x, 2.0f, 0.05f);
}

TEST_F(PhysicsSimulationTest, ImpulseChangesVelocity)
{
    Entity sphere = create_sphere(vec3(0.0f, 10.0f, 0.0f));
    body(sphere)->use_gravity = false;
    body(sphere)->mass = 2.0f;

    step(1);

    Force impulse;
    impulse.mode = ForceMode::IMPULSE;
    impulse.force = vec3(0.0f, 0.0f, 4.0f);
    body(sphere)->forces.push_back(impulse);

    step(1);

    EXPECT_TRUE(body(sphere)->forces.empty());
    EXPECT_NEAR(body(sphere)->linear_velocity.get().z, 2.0f, 0.05f);
    EXPECT_GT(position(sphere).z, 0.0f);
}

TEST_F(PhysicsSimulationTest, BodyRestsOnGround)
{
    Entity ground = create_ground();
    Entity sphere = create_sphere(vec3(0.0f, 3.0f, 0.0f));

    step(600);

    EXPECT_NEAR(position(sphere).y, 0.5f, 0.05f);
    EXPECT_NEAR(body(sphere)->linear_velocity.get().y, 0.0f, 0.1f);
}

TEST_F(PhysicsSimulationTest, CollisionFilterLetsBodyPassThrough)
{
    Entity ground = create_ground(CollisionType::COLLISION_2);
    Entity sphere = create_sphere(vec3(0.0f, 3.0f, 0.0f));

    step(180);

    EXPECT_LT(position(sphere).y, -2.0f);
}

TEST_F(PhysicsSimulationTest, CollisionEventsAreReported)
{
    Entity ground = create_ground();
    Entity sphere = create_sphere(vec3(0.0f, 1.0f, 0.0f));

    const BodyComponent* sphere_body = body(sphere);
    const BodyComponent* ground_body = ground.get_component<StaticBodyComponent>();

    bool reported = false;
    for (uint32 i = 0; i < 120 && !reported; ++i)
    {
        step(1);

        const ref<Physics> physics_core = PhysicsEngine::get_physics_core();
        for (const Collision& collision : physics_core->simulation_event_callback->new_collisions)
        {
            const bool sphere_vs_ground = collision.this_actor == sphere_body && collision.other_actor == ground_body;
            const bool ground_vs_sphere = collision.this_actor == ground_body && collision.other_actor == sphere_body;

            if (sphere_vs_ground || ground_vs_sphere)
            {
                reported = true;
                EXPECT_GT(collision.contacts_count, 0);
                EXPECT_GT(collision.impulse.magnitude(), 0.0f);
            }
        }
    }

    EXPECT_TRUE(reported);
}

TEST_F(PhysicsSimulationTest, RaycastHitsGround)
{
    Entity ground = create_ground();

    step(1);

    RaycastQuery::Params params;
    params.query_ray.origin = vec3(1.0f, 5.0f, 2.0f);
    params.query_ray.direction = vec3(0.0f, -1.0f, 0.0f);
    params.distance = 10.0f;

    SceneQueryPositionedHit hit;
    ASSERT_TRUE(RaycastQuery::closest(world, params, &hit));
    EXPECT_NEAR(hit.distance, 5.0f, 1e-3f);
    EXPECT_TRUE(fuzzy_equals(hit.position, vec3(1.0f, 0.0f, 2.0f), 1e-3f));
    EXPECT_TRUE(fuzzy_equals(hit.normal, vec3(0.0f, 1.0f, 0.0f), 1e-3f));
    EXPECT_EQ(hit.shape_component, ground.get_component<BoxShapeComponent>());

    params.query_ray.origin = vec3(20.0f, 5.0f, 0.0f);
    EXPECT_FALSE(RaycastQuery::closest(world, params, &hit));

    params.query_ray.origin = vec3(1.0f, 5.0f, 2.0f);
    params.distance = 4.0f;
    EXPECT_FALSE(RaycastQuery::closest(world, params, &hit));
}

TEST_F(PhysicsSimulationTest, RaycastSeesDynamicBodies)
{
    Entity sphere = create_sphere(vec3(0.0f, 2.0f, 0.0f));
    body(sphere)->use_gravity = false;

    step(1);

    RaycastQuery::Params params;
    params.query_ray.origin = vec3(0.0f, 2.0f, -5.0f);
    params.query_ray.direction = vec3(0.0f, 0.0f, 1.0f);
    params.distance = 10.0f;

    SceneQueryPositionedHit hit;
    ASSERT_TRUE(RaycastQuery::closest(world, params, &hit));
    EXPECT_NEAR(hit.distance, 4.5f, 1e-3f);
    EXPECT_EQ(hit.shape_component, sphere.get_component<SphereShapeComponent>());

    params.filter_flags = SceneQueryFilterFlag::STATICS;
    EXPECT_FALSE(RaycastQuery::closest(world, params, &hit));
}

namespace
{
    using HeightFunction = uint16 (*)(uint32 global_x, uint32 global_z);

    void fill_terrain_heights(TerrainComponent* terrain, HeightFunction height)
    {
        const uint32 vertices_per_chunk_dim = TerrainComponent::get_vertices_per_chunk_dim();
        const uint32 segments_per_chunk_dim = vertices_per_chunk_dim - 1;

        for (uint32 cz = 0; cz < terrain->chunks_per_dim; ++cz)
        {
            for (uint32 cx = 0; cx < terrain->chunks_per_dim; ++cx)
            {
                std::vector<uint16>& heights = terrain->chunk(cx, cz).heights;
                heights.resize(vertices_per_chunk_dim * vertices_per_chunk_dim);

                for (uint32 z = 0; z < vertices_per_chunk_dim; ++z)
                {
                    for (uint32 x = 0; x < vertices_per_chunk_dim; ++x)
                    {
                        heights[z * vertices_per_chunk_dim + x] = height(cx * segments_per_chunk_dim + x, cz * segments_per_chunk_dim + z);
                    }
                }
            }
        }
    }

    uint16 linear_height(uint32 x, uint32 z)
    {
        return uint16(x * 100 + z * 50);
    }

    uint16 raised_linear_height(uint32 x, uint32 z)
    {
        return uint16(linear_height(x, z) + 1000);
    }

    uint16 product_height(uint32 x, uint32 z)
    {
        return uint16(x * z * 2);
    }

    uint16 flat_height(uint32, uint32)
    {
        return 30000;
    }

    bool raycast_down(World* world, const vec3& position, SceneQueryPositionedHit* hit)
    {
        RaycastQuery::Params params;
        params.query_ray.origin = position + vec3(0.0f, 100.0f, 0.0f);
        params.query_ray.direction = vec3(0.0f, -1.0f, 0.0f);
        params.distance = 200.0f;
        return RaycastQuery::closest(world, params, hit);
    }
}

TEST_F(PhysicsSimulationTest, TerrainShapeMatchesTerrainHeights)
{
    constexpr float chunk_size = 8.0f;
    constexpr float amplitude = 10.0f;
    const vec3 terrain_position = vec3(3.0f, 1.0f, -2.0f);

    Entity terrain_entity = world->create_entity("Terrain");
    terrain_entity.get_component<TransformComponent>()->set_world_position(terrain_position);

    TerrainComponent* terrain = terrain_entity.add_component<TerrainComponent>(2u, chunk_size, amplitude, nullptr, nullptr, nullptr);
    fill_terrain_heights(terrain, &linear_height);

    TerrainShapeComponent* shape = terrain_entity.add_component<TerrainShapeComponent>();
    shape->collision_type = CollisionType::TERRAIN;
    terrain_entity.add_component<StaticBodyComponent>()->simulated = true;

    step(1);

    ASSERT_NE(shape->get_shape(), nullptr);
    ASSERT_EQ(shape->get_geometry().getType(), physx::PxGeometryType::eHEIGHTFIELD);
    const physx::PxHeightFieldGeometry& geometry = static_cast<const physx::PxHeightFieldGeometry&>(shape->get_geometry());
    EXPECT_EQ(geometry.heightField->getNbRows(), 257u);
    EXPECT_EQ(geometry.heightField->getNbColumns(), 257u);
    EXPECT_EQ(geometry.heightField->getReferenceCount(), 1u);

    const float spacing = chunk_size / float(TerrainComponent::get_vertices_per_chunk_dim() - 1);
    const vec3 min_corner = terrain->get_min_corner(terrain_position);

    const vec2 grid_points[] = {vec2(0.25f, 0.5f), vec2(17.0f, 3.0f), vec2(128.0f, 40.0f), vec2(70.5f, 128.25f), vec2(200.3f, 31.7f), vec2(255.0f, 250.5f)};
    for (const vec2& grid_point : grid_points)
    {
        const vec3 position = min_corner + vec3(grid_point.x * spacing, 0.0f, grid_point.y * spacing);
        const float expected_y = terrain_position.y + (grid_point.x * 100.0f + grid_point.y * 50.0f) / 65535.0f * amplitude;

        SceneQueryPositionedHit hit;
        ASSERT_TRUE(raycast_down(world, position, &hit));
        EXPECT_NEAR(hit.position.y, expected_y, 1e-3f) << grid_point.x << ", " << grid_point.y;
        EXPECT_EQ(hit.shape_component, shape);
    }

    SceneQueryPositionedHit hit;
    EXPECT_FALSE(raycast_down(world, min_corner + vec3(-0.5f, 0.0f, 1.0f), &hit));
    EXPECT_FALSE(raycast_down(world, min_corner + vec3(1.0f, 0.0f, 2.0f * chunk_size + 0.5f), &hit));
}

TEST_F(PhysicsSimulationTest, TerrainShapeSplitsQuadsLikeRenderer)
{
    constexpr float amplitude = 1000.0f;

    Entity terrain_entity = world->create_entity("Terrain");
    TerrainComponent* terrain = terrain_entity.add_component<TerrainComponent>(1u, 16.0f, amplitude, nullptr, nullptr, nullptr);
    fill_terrain_heights(terrain, &product_height);

    terrain_entity.add_component<TerrainShapeComponent>()->collision_type = CollisionType::TERRAIN;
    terrain_entity.add_component<StaticBodyComponent>()->simulated = true;

    step(1);

    const float spacing = 16.0f / float(TerrainComponent::get_vertices_per_chunk_dim() - 1);
    const vec3 min_corner = terrain->get_min_corner(vec3::zero);

    // Terrain index buffers split the quad (x, z) along (x + 1, z) - (x, z + 1).
    const uint32 x = 10;
    const uint32 z = 20;
    const float expected_center_height = 0.5f * float(product_height(x + 1, z) + product_height(x, z + 1)) / 65535.0f * amplitude;

    SceneQueryPositionedHit hit;
    ASSERT_TRUE(raycast_down(world, min_corner + vec3((x + 0.5f) * spacing, 0.0f, (z + 0.5f) * spacing), &hit));
    EXPECT_NEAR(hit.position.y, expected_center_height, 1e-3f);
}

TEST_F(PhysicsSimulationTest, TerrainShapeIsCreatedWhenHeightsAppearAndRebuiltOnChange)
{
    Entity terrain_entity = world->create_entity("Terrain");
    TerrainComponent* terrain = terrain_entity.add_component<TerrainComponent>(1u, 8.0f, 10.0f, nullptr, nullptr, nullptr);
    TerrainShapeComponent* shape = terrain_entity.add_component<TerrainShapeComponent>();
    shape->collision_type = CollisionType::TERRAIN;
    StaticBodyComponent* body_component = terrain_entity.add_component<StaticBodyComponent>();
    body_component->simulated = true;

    step(1);

    ASSERT_NE(body_component->get_rigid_actor(), nullptr);
    EXPECT_EQ(shape->get_shape(), nullptr);

    fill_terrain_heights(terrain, &linear_height);
    terrain->mark_heights_changed();

    step(1);

    ASSERT_NE(shape->get_shape(), nullptr);
    EXPECT_EQ(body_component->get_rigid_actor()->getNbShapes(), 1u);

    const vec3 probe = terrain->get_min_corner(vec3::zero) + vec3(2.0f, 0.0f, 3.0f);
    const float spacing = 8.0f / float(TerrainComponent::get_vertices_per_chunk_dim() - 1);
    const float expected_y = (2.0f / spacing * 100.0f + 3.0f / spacing * 50.0f) / 65535.0f * 10.0f;

    SceneQueryPositionedHit hit;
    ASSERT_TRUE(raycast_down(world, probe, &hit));
    EXPECT_NEAR(hit.position.y, expected_y, 1e-3f);
    EXPECT_EQ(hit.shape_component, shape);

    physx::PxShape* created_shape = shape->get_shape();

    fill_terrain_heights(terrain, &raised_linear_height);
    terrain->mark_heights_changed();

    step(1);

    EXPECT_EQ(shape->get_shape(), created_shape);
    ASSERT_TRUE(raycast_down(world, probe, &hit));
    EXPECT_NEAR(hit.position.y, expected_y + 1000.0f / 65535.0f * 10.0f, 1e-3f);
}

TEST_F(PhysicsSimulationTest, BodyRestsOnTerrain)
{
    Entity terrain_entity = world->create_entity("Terrain");
    TerrainComponent* terrain = terrain_entity.add_component<TerrainComponent>(1u, 16.0f, 10.0f, nullptr, nullptr, nullptr);
    fill_terrain_heights(terrain, &flat_height);

    terrain_entity.add_component<TerrainShapeComponent>()->collision_type = CollisionType::TERRAIN;
    terrain_entity.add_component<StaticBodyComponent>()->simulated = true;

    Entity sphere = create_sphere(vec3(-1.0f, 10.0f, 2.0f));

    step(600);

    EXPECT_NEAR(position(sphere).y, 30000.0f / 65535.0f * 10.0f + 0.5f, 0.05f);
}

TEST_F(PhysicsSimulationTest, PlaneShapeUsesEntityAndLocalPose)
{
    Entity plane = world->create_entity("Plane");
    plane.get_component<TransformComponent>()->set_world_position(vec3(0.0f, -9.0f, 0.0f));
    PlaneShapeComponent* shape = plane.add_component<PlaneShapeComponent>();
    shape->local_position = vec3(0.0f, 4.0f, 0.0f);
    plane.add_component<StaticBodyComponent>()->simulated = true;

    step(1);

    ASSERT_NE(shape->get_shape(), nullptr);
    EXPECT_EQ(shape->collision_type.get(), CollisionType::TERRAIN);

    SceneQueryPositionedHit hit;
    ASSERT_TRUE(raycast_down(world, vec3(37.0f, 0.0f, -120.0f), &hit));
    EXPECT_NEAR(hit.position.y, -5.0f, 1e-3f);
    EXPECT_TRUE(fuzzy_equals(hit.normal, vec3(0.0f, 1.0f, 0.0f), 1e-3f));
    EXPECT_EQ(hit.shape_component, shape);

    Entity sphere = create_sphere(vec3(0.0f, 0.0f, 0.0f));

    step(600);

    EXPECT_NEAR(position(sphere).y, -4.5f, 0.05f);
}

TEST_F(PhysicsSimulationTest, PlaneShapeFollowsEntityRotation)
{
    Entity wall = world->create_entity("Wall");
    wall.get_component<TransformComponent>()->set_world_transform(trs(vec3(0.0f, 0.0f, 5.0f), quat(vec3(1.0f, 0.0f, 0.0f), deg2rad(-90.0f)), vec3(1.0f)));
    PlaneShapeComponent* shape = wall.add_component<PlaneShapeComponent>();
    wall.add_component<StaticBodyComponent>()->simulated = true;

    step(1);

    RaycastQuery::Params params;
    params.query_ray.origin = vec3(1.0f, 2.0f, 0.0f);
    params.query_ray.direction = vec3(0.0f, 0.0f, 1.0f);
    params.distance = 10.0f;

    SceneQueryPositionedHit hit;
    ASSERT_TRUE(RaycastQuery::closest(world, params, &hit));
    EXPECT_NEAR(hit.distance, 5.0f, 1e-3f);
    EXPECT_TRUE(fuzzy_equals(hit.normal, vec3(0.0f, 0.0f, -1.0f), 1e-3f));
    EXPECT_EQ(hit.shape_component, shape);
}

TEST_F(PhysicsSimulationTest, BodyDestroyedBeforeCreationIsIgnored)
{
    Entity sphere = create_sphere(vec3(0.0f, 10.0f, 0.0f));
    Entity other = create_sphere(vec3(2.0f, 10.0f, 0.0f));

    world->destroy_entity(sphere);

    step(10);

    EXPECT_NE(body(other)->get_rigid_actor(), nullptr);
    EXPECT_LT(position(other).y, 10.0f);
}

TEST_F(PhysicsSimulationTest, RemovingRestingBodyKeepsSimulationValid)
{
    Entity ground = create_ground();
    Entity sphere = create_sphere(vec3(0.0f, 0.6f, 0.0f));
    Entity falling = create_sphere(vec3(3.0f, 5.0f, 0.0f));

    step(60);

    world->destroy_entity(sphere);

    step(600);

    EXPECT_NEAR(position(falling).y, 0.5f, 0.05f);
}

TEST_F(PhysicsSimulationTest, DestroyingBodyKeepsOthersSimulating)
{
    Entity first = create_sphere(vec3(-3.0f, 10.0f, 0.0f));
    Entity second = create_sphere(vec3(0.0f, 10.0f, 0.0f));
    Entity third = create_sphere(vec3(3.0f, 10.0f, 0.0f));

    step(10);

    world->destroy_entity(first);

    step(50);

    const vec3 second_position = position(second);
    const vec3 third_position = position(third);

    EXPECT_NEAR(second_position.y, 10.0f - 0.5f * 9.81f, 0.15f);
    EXPECT_NEAR(third_position.y, second_position.y, 1e-3f);
    EXPECT_NEAR(second_position.x, 0.0f, 1e-3f);
    EXPECT_NEAR(third_position.x, 3.0f, 1e-3f);

    EXPECT_NE(second.get_component<SphereShapeComponent>()->get_shape(), nullptr);
    EXPECT_EQ(body(second)->get_rigid_actor()->userData, static_cast<void*>(body(second)));
    EXPECT_EQ(body(third)->get_rigid_actor()->userData, static_cast<void*>(body(third)));
}

namespace
{
    class VehicleSimulationTest : public PhysicsSimulationTest
    {
    protected:
        void SetUp() override
        {
            PhysicsSimulationTest::SetUp();

            world->get_root_component<CollisionsHolderRootComponent>()->set_collision_filter(to_bits(CollisionType::VEHICLE), to_bits(CollisionType::TERRAIN), true);
            world->get_root_component<CollisionsHolderRootComponent>()->set_collision_filter(to_bits(CollisionType::VEHICLE), to_bits(CollisionType::RAGDOLL), true);

            Entity ground = world->create_entity("Ground");
            ground.get_component<TransformComponent>()->set_world_position(vec3(0.0f, -0.5f, 0.0f));

            BoxShapeComponent* shape = ground.add_component<BoxShapeComponent>();
            shape->half_extents = vec3(500.0f, 0.5f, 500.0f);
            shape->collision_type = CollisionType::TERRAIN;

            ground.add_component<StaticBodyComponent>()->simulated = true;
        }

        std::vector<rttr::type> get_system_types() const override
        {
            std::vector<rttr::type> types = PhysicsSimulationTest::get_system_types();
            types.push_back(rttr::type::get_by_name("VehicleSystem"));
            return types;
        }

        W4VehicleComponent* create_vehicle(const vec3& position = vec3(0.0f, 0.3f, 0.0f))
        {
            Entity entity = world->create_entity("Vehicle");
            entity.get_component<TransformComponent>()->set_world_position(position);
            return entity.add_component<W4VehicleComponent>();
        }

        static vec3 vehicle_position(W4VehicleComponent* vehicle)
        {
            return vehicle->get_entity().get_component<TransformComponent>()->get_world_position();
        }

        static vec3 up(W4VehicleComponent* vehicle)
        {
            return vehicle->get_entity().get_component<TransformComponent>()->get_world_rotation() * vec3(0.0f, 1.0f, 0.0f);
        }

        static vec3 forward(W4VehicleComponent* vehicle)
        {
            return vehicle->get_entity().get_component<TransformComponent>()->get_world_rotation() * vec3(0.0f, 0.0f, 1.0f);
        }
    };
}

TEST_F(VehicleSimulationTest, DefaultVehicleSettlesOnWheels)
{
    W4VehicleComponent* vehicle = create_vehicle();

    step(120);

    ASSERT_TRUE(vehicle->is_created());
    EXPECT_EQ(vehicle->get_debug_data().grounded_wheels, 4u);
    EXPECT_NEAR(vehicle_position(vehicle).y, 0.0f, 0.05f);
    EXPECT_GT(up(vehicle).y, 0.99f);
    EXPECT_NEAR(vehicle->get_debug_data().forward_speed, 0.0f, 0.05f);
}

TEST_F(VehicleSimulationTest, InvalidConfigIsRejected)
{
    W4VehicleComponent* vehicle = create_vehicle();
    vehicle->config.axles.resize(1);

    step(2);

    EXPECT_FALSE(vehicle->is_created());

    vehicle->config = VehicleConfig::make_default();

    step(2);

    EXPECT_TRUE(vehicle->is_created());
}

TEST_F(VehicleSimulationTest, InvalidConfigChangeIsReverted)
{
    W4VehicleComponent* vehicle = create_vehicle();
    step(2);
    ASSERT_TRUE(vehicle->is_created());

    vehicle->config.chassis.mass = 0.0f;
    step(2);

    EXPECT_TRUE(vehicle->is_created());
    EXPECT_TRUE(vehicle->config == VehicleConfig::make_default());

    vehicle->config.chassis.mass = 2000.0f;
    step(2);

    EXPECT_TRUE(vehicle->is_created());
    EXPECT_EQ(vehicle->config.chassis.mass, 2000.0f);
}

TEST_F(VehicleSimulationTest, RaycastHitsVehicleShape)
{
    W4VehicleComponent* vehicle = create_vehicle();
    step(60);

    RaycastQuery::Params params;
    params.query_ray.origin = vec3(0.0f, 5.0f, 0.0f);
    params.query_ray.direction = vec3(0.0f, -1.0f, 0.0f);
    params.distance = 10.0f;

    SceneQueryFilter filter(to_bits(CollisionType::VEHICLE), 0);
    params.filter_callback = &filter;
    params.filter_flags |= SceneQueryFilterFlag::PREFILTER;

    const ShapeComponent* chassis_shape = vehicle->get_entity().get_component_if_exists<VehicleShapeComponent>();
    ASSERT_NE(chassis_shape, nullptr);
    ASSERT_NE(chassis_shape->get_shape(), nullptr);

    SceneQueryPositionedHit hit;
    ASSERT_TRUE(RaycastQuery::closest(world, params, &hit));
    EXPECT_EQ(hit.shape_component, chassis_shape);
    EXPECT_GT(hit.position.y, 0.5f);

    filter.ignore_list.push_back(EntityPtr{vehicle->get_entity()});
    EXPECT_FALSE(RaycastQuery::closest(world, params, &hit));
}

TEST_F(VehicleSimulationTest, WheelShapeBelongsToWheelEntity)
{
    W4VehicleComponent* vehicle = create_vehicle();

    Entity wheel = world->create_entity("Wheel");
    wheel.set_parent(vehicle->get_entity().get_handle());
    wheel.add_component<VehicleWheelComponent>(1u);

    step(60);

    // Below the chassis, through the front left wheel.
    RaycastQuery::Params params;
    params.query_ray.origin = vehicle_position(vehicle) + vec3(5.0f, 0.1f, 1.43f);
    params.query_ray.direction = vec3(-1.0f, 0.0f, 0.0f);
    params.distance = 10.0f;
    params.filter_flags = SceneQueryFilterFlag::DYNAMICS;

    SceneQueryPositionedHit hit;
    ASSERT_TRUE(RaycastQuery::closest(world, params, &hit));
    ASSERT_NE(hit.shape_component, nullptr);
    EXPECT_EQ(hit.shape_component->get_entity(), wheel);

    world->destroy_entity(wheel);
    step(1);

    ASSERT_TRUE(RaycastQuery::closest(world, params, &hit));
    EXPECT_EQ(hit.shape_component, vehicle->get_entity().get_component_if_exists<VehicleShapeComponent>());

    world->destroy_entity(vehicle->get_entity());
    step(1);

    EXPECT_FALSE(RaycastQuery::closest(world, params, &hit));
}

TEST_F(VehicleSimulationTest, AcceleratesAndShiftsUp)
{
    W4VehicleComponent* vehicle = create_vehicle();
    step(60);

    vehicle->driver_input.throttle = 1.0f;
    step(300);

    EXPECT_GT(vehicle->get_debug_data().forward_speed, 15.0f);
    EXPECT_GE(vehicle->get_debug_data().gear, 2);
    EXPECT_GT(up(vehicle).y, 0.95f);
    EXPECT_NEAR(vehicle_position(vehicle).x, 0.0f, 1.0f);
}

TEST_F(VehicleSimulationTest, BrakeStopsThenReverses)
{
    W4VehicleComponent* vehicle = create_vehicle();
    step(60);

    vehicle->driver_input.throttle = 1.0f;
    step(180);
    ASSERT_GT(vehicle->get_debug_data().forward_speed, 10.0f);

    vehicle->driver_input.throttle = 0.0f;
    vehicle->driver_input.brake = 1.0f;

    float min_abs_speed = FLT_MAX;
    for (uint32 i = 0; i < 240; ++i)
    {
        step(1);
        min_abs_speed = std::min(min_abs_speed, std::abs(vehicle->get_debug_data().forward_speed));
    }
    EXPECT_LT(min_abs_speed, 1.0f);
    EXPECT_GT(up(vehicle).y, 0.95f);

    step(180);
    EXPECT_LT(vehicle->get_debug_data().forward_speed, -1.0f);
    EXPECT_TRUE(vehicle->get_debug_data().commands.reverse);
}

TEST_F(VehicleSimulationTest, SteersLeftAtSpeedWithoutRollover)
{
    W4VehicleComponent* vehicle = create_vehicle();
    step(60);

    vehicle->driver_input.throttle = 1.0f;
    step(240);

    vehicle->driver_input.throttle = 0.5f;
    vehicle->driver_input.steer = 1.0f;
    for (uint32 i = 0; i < 90; ++i)
    {
        step(1);
        ASSERT_GT(up(vehicle).y, 0.8f);
    }

    EXPECT_GT(forward(vehicle).x, 0.3f);
    EXPECT_EQ(vehicle->get_debug_data().grounded_wheels, 4u);
}

TEST_F(VehicleSimulationTest, ConvexHullChassisSettlesAndDrives)
{
    W4VehicleComponent* vehicle = create_vehicle();
    for (uint32 i = 0; i < 8; ++i)
    {
        vehicle->config.chassis.collision_hull.push_back(vec3((i & 1) ? 0.85f : -0.85f, (i & 2) ? 1.4f : 0.25f, (i & 4) ? 2.3f : -2.3f));
    }

    step(120);

    ASSERT_TRUE(vehicle->is_created());
    EXPECT_EQ(vehicle->get_debug_data().grounded_wheels, 4u);
    EXPECT_NEAR(vehicle_position(vehicle).y, 0.0f, 0.05f);

    vehicle->driver_input.throttle = 1.0f;
    step(180);

    EXPECT_GT(vehicle->get_debug_data().forward_speed, 8.0f);
    EXPECT_GT(up(vehicle).y, 0.95f);
}

TEST_F(VehicleSimulationTest, DoorOpensAndSteeringWheelFollowsSteer)
{
    W4VehicleComponent* vehicle = create_vehicle();

    Entity door = world->create_entity("Door");
    door.set_parent(vehicle->get_entity().get_handle());
    door.get_component<TransformComponent>()->set_local_transform(trs(vec3(0.9f, 0.8f, 0.85f), quat::identity, vec3(1.0f)));
    VehicleDoorComponent* door_component = door.add_component<VehicleDoorComponent>();
    door_component->max_angle = -70.0f;
    for (uint32 i = 0; i < 8; ++i)
    {
        door_component->collision_hull.push_back(vec3((i & 1) ? 0.0f : -0.1f, (i & 2) ? 0.45f : -0.45f, (i & 4) ? 0.0f : -1.1f));
    }

    Entity steering = world->create_entity("Steering");
    steering.set_parent(vehicle->get_entity().get_handle());
    steering.get_component<TransformComponent>()->set_local_transform(trs(vec3(0.4f, 0.95f, 0.5f), quat::identity, vec3(1.0f)));
    steering.add_component<VehicleSteeringWheelComponent>();

    step(60);
    door_component->open = 1.0f;
    vehicle->driver_input.steer = 0.1f;
    step(120);

    const quat door_rotation = door.get_component<TransformComponent>()->get_local_transform().rotation;
    const vec3 door_forward = door_rotation * vec3(0.0f, 0.0f, -1.0f);
    EXPECT_NEAR(std::atan2(door_forward.x, -door_forward.z), deg2rad(70.0f), 1e-3f);
    EXPECT_TRUE(fuzzy_equals(door.get_component<TransformComponent>()->get_local_transform().position, vec3(0.9f, 0.8f, 0.85f)));

    const float steer_angle = vehicle->get_debug_data().steer_angle;
    EXPECT_GT(steer_angle, 0.02f);
    const quat wheel_rotation = steering.get_component<TransformComponent>()->get_local_transform().rotation;
    const vec3 rim_up = wheel_rotation * vec3(0.0f, 1.0f, 0.0f);
    EXPECT_GT(rim_up.x, 0.0f);
    EXPECT_GT(vehicle_position(vehicle).y, -0.1f);
}

TEST_F(VehicleSimulationTest, ConvexShapesAreGpuCompatibleInGpuScene)
{
    if (!PhysicsEngine::get_physics_core()->is_gpu())
    {
        GTEST_SKIP() << "CPU physics scene";
    }

    // Dense hulls: cooked without GPU data the door hull has ~130 vertices and is not GPU compatible.
    auto ellipsoid_points = [](const vec3& center, const vec3& radius, uint32 count)
    {
        std::vector<vec3> points;
        for (uint32 i = 0; i < count; ++i)
        {
            const float y = 1.0f - 2.0f * (float(i) + 0.5f) / float(count);
            const float r = std::sqrt(1.0f - y * y);
            const float phi = float(i) * 2.399963f;
            points.push_back(center + vec3(r * std::cos(phi), y, r * std::sin(phi)) * radius);
        }
        return points;
    };

    W4VehicleComponent* vehicle = create_vehicle();
    vehicle->config.chassis.collision_hull = ellipsoid_points(vehicle->config.chassis.collision_center, vehicle->config.chassis.collision_half_extents, 200);

    Entity door = world->create_entity("Door");
    door.set_parent(vehicle->get_entity().get_handle());
    door.get_component<TransformComponent>()->set_local_transform(trs(vec3(0.9f, 0.8f, 0.85f), quat::identity, vec3(1.0f)));
    VehicleDoorComponent* door_component = door.add_component<VehicleDoorComponent>();
    door_component->collision_hull = ellipsoid_points(vec3(-0.05f, 0.0f, -0.55f), vec3(0.05f, 0.45f, 0.55f), 200);

    step(2);
    ASSERT_TRUE(vehicle->is_created());

    RaycastQuery::Params params;
    params.query_ray.origin = vehicle_position(vehicle) + vec3(0.0f, 10.0f, 0.0f);
    params.query_ray.direction = vec3(0.0f, -1.0f, 0.0f);
    params.distance = 20.0f;
    SceneQueryPositionedHit hit;
    ASSERT_TRUE(RaycastQuery::closest(world, params, &hit));
    ASSERT_NE(hit.shape_component, nullptr);
    physx::PxRigidActor* actor = hit.shape_component->get_shape()->getActor();
    physx::PxShape* shapes[16];
    const physx::PxU32 count = actor->getShapes(shapes, 16);

    uint32 convex_count = 0;
    for (physx::PxU32 i = 0; i < count; ++i)
    {
        if (shapes[i]->getGeometry().getType() != physx::PxGeometryType::eCONVEXMESH)
        {
            continue;
        }
        const physx::PxConvexMesh* mesh = static_cast<const physx::PxConvexMeshGeometry&>(shapes[i]->getGeometry()).convexMesh;
        EXPECT_TRUE(mesh->isGpuCompatible()) << "shape " << i << ": " << mesh->getNbVertices() << " vertices, " << mesh->getNbPolygons() << " polygons";
        ++convex_count;
    }

    EXPECT_EQ(convex_count, 6u);
}

TEST_F(VehicleSimulationTest, WheelHubSteersWithoutSpinning)
{
    W4VehicleComponent* vehicle = create_vehicle();

    Entity wheel = world->create_entity("Wheel");
    wheel.set_parent(vehicle->get_entity().get_handle());
    wheel.add_component<VehicleWheelComponent>(1u);

    Entity hub = world->create_entity("Hub");
    hub.set_parent(vehicle->get_entity().get_handle());
    hub.add_component<VehicleWheelComponent>(1u)->should_spin = false;

    step(60);
    vehicle->driver_input.throttle = 1.0f;
    vehicle->driver_input.steer = 0.3f;
    step(37);

    const trs& wheel_transform = wheel.get_component<TransformComponent>()->get_local_transform();
    const trs& hub_transform = hub.get_component<TransformComponent>()->get_local_transform();
    EXPECT_TRUE(fuzzy_equals(wheel_transform.position, hub_transform.position));

    const vec3 hub_forward = hub_transform.rotation * vec3(0.0f, 0.0f, 1.0f);
    EXPECT_NEAR(hub_forward.y, 0.0f, 1e-4f);
    EXPECT_GT(hub_forward.x, 0.02f);

    const vec3 wheel_forward = wheel_transform.rotation * vec3(0.0f, 0.0f, 1.0f);
    EXPECT_GT(std::abs(wheel_forward.y), 0.05f);
    EXPECT_TRUE(fuzzy_equals(wheel_transform.rotation * vec3(1.0f, 0.0f, 0.0f), hub_transform.rotation * vec3(1.0f, 0.0f, 0.0f)));
}

TEST_F(VehicleSimulationTest, ParkedVehicleBlocksDynamicBody)
{
    W4VehicleComponent* vehicle = create_vehicle();
    step(180);

    Entity sphere = create_sphere(vec3(4.0f, 0.8f, 0.0f), 0.25f, CollisionType::RAGDOLL);
    body(sphere)->use_gravity = false;
    body(sphere)->linear_velocity = vec3(-6.0f, 0.0f, 0.0f);

    step(90);

    EXPECT_GT(position(sphere).x, 0.9f);
}

TEST_F(VehicleSimulationTest, ParkedVehicleReportsKinematicContact)
{
    W4VehicleComponent* vehicle = create_vehicle();
    step(180);

    Entity box = world->create_entity("Limb");
    box.get_component<TransformComponent>()->set_world_position(vec3(3.0f, 0.8f, 0.0f));
    BoxShapeComponent* shape = box.add_component<BoxShapeComponent>();
    shape->half_extents = vec3(0.1f, 0.1f, 0.1f);
    shape->collision_type = CollisionType::RAGDOLL;
    DynamicBodyComponent* limb = box.add_component<DynamicBodyComponent>();
    limb->simulated = true;
    limb->kinematic = true;
    limb->kinematic_motion_type = KinematicMotionType::VELOCITY;

    step(2);

    bool touched_vehicle = false;
    for (uint32 i = 0; i < 90 && !touched_vehicle; ++i)
    {
        PhysicsUtils::manual_set_physics_transform_locked(box, vec3(3.0f - i * 0.04f, 0.8f, 0.0f), quat::identity);
        step(1);

        for (const Collision& collision : PhysicsEngine::get_physics_core()->simulation_event_callback->new_collisions)
        {
            const Component* a = collision.this_actor;
            const Component* b = collision.other_actor;
            touched_vehicle |= (a == vehicle && b == limb) || (a == limb && b == vehicle);
        }
    }

    EXPECT_TRUE(touched_vehicle);
}


TEST_F(VehicleSimulationTest, ParkedVehicleWheelBlocksDynamicBody)
{
    W4VehicleComponent* vehicle = create_vehicle();
    step(180);

    Entity sphere = create_sphere(vec3(3.0f, 0.08f, 1.43f), 0.06f, CollisionType::RAGDOLL);
    body(sphere)->use_gravity = false;
    body(sphere)->linear_velocity = vec3(-4.0f, 0.0f, 0.0f);

    step(90);

    EXPECT_GT(position(sphere).x, 0.9f);
}

TEST_F(VehicleSimulationTest, FallingVehicleAcceleratesAtGravity)
{
    W4VehicleComponent* vehicle = create_vehicle(vec3(0.0f, 20.0f, 0.0f));
    step(2);

    const VehicleShapeComponent* chassis_shape = vehicle->get_entity().get_component_if_exists<VehicleShapeComponent>();
    ASSERT_NE(chassis_shape, nullptr);
    const physx::PxRigidDynamic* actor = chassis_shape->get_shape()->getActor()->is<physx::PxRigidDynamic>();

    const float start_velocity = actor->getLinearVelocity().y;
    step(10);
    EXPECT_NEAR(actor->getLinearVelocity().y - start_velocity, -9.81f * 10.0f * fixed_dt, 0.02f);
}
