#include <gtest/gtest.h>

#include <physics_module.h>

#include <physics/body_component.h>
#include <physics/collision_types.h>
#include <physics/collisions_holder_root_component.h>
#include <physics/scene_queries.h>
#include <physics/shape_component.h>
#include <physics/core/physics.h>
#include <physics/core/physics_types.h>

#include <ecs/world.h>
#include <ecs/world_system_scheduler.h>
#include <ecs/base_components/transform_component.h>

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

            std::vector<rttr::type> types = {rttr::type::get_by_name("PhysicsSystem"), rttr::type::get_by_name("ShapeSystem")};
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
