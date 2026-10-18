#include <gtest/gtest.h>

#include <ecs/world.h>
#include <ecs/base_components/transform_component.h>

namespace
{
    using namespace era_engine;

    class TransformHierarchyTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            world = new World("GameWorld");
            world->init();

            parent = world->create_entity("Parent");
            child = world->create_entity("Child");
            grandchild = world->create_entity("Grandchild");

            child.set_parent(parent.get_handle());
            grandchild.set_parent(child.get_handle());
        }

        void TearDown() override
        {
            delete world;
        }

        TransformComponent* transform(Entity& entity)
        {
            return entity.get_component<TransformComponent>();
        }

        World* world = nullptr;
        Entity parent;
        Entity child;
        Entity grandchild;
    };
}

TEST(ECS_Transform, DefaultIsIdentity)
{
    World* world = new World("GameWorld");
    world->init();

    Entity entity = world->create_entity();
    const TransformComponent* transform = entity.get_component<TransformComponent>();

    EXPECT_EQ(transform->type, TransformComponent::DYNAMIC);
    EXPECT_TRUE(fuzzy_equals(transform->get_world_transform(), trs::identity));
    EXPECT_TRUE(fuzzy_equals(transform->get_local_transform(), trs::identity));

    delete world;
}

TEST(ECS_Transform, ConstructorTransformIsKept)
{
    World* world = new World("GameWorld");
    world->init();

    Entity entity = world->create_entity();
    const trs initial = trs(vec3(1.0f, 2.0f, 3.0f), quat::identity, vec3(2.0f, 2.0f, 2.0f));

    TransformComponent transform(entity.get_data_weakref().lock(), initial, TransformComponent::STATIC);

    EXPECT_EQ(transform.type, TransformComponent::STATIC);
    EXPECT_TRUE(fuzzy_equals(transform.get_local_transform(), initial));
    EXPECT_TRUE(fuzzy_equals(transform.get_world_transform(), initial));

    delete world;
}

TEST(ECS_Transform, WorldEqualsLocalUnderRoot)
{
    World* world = new World("GameWorld");
    world->init();

    Entity entity = world->create_entity();
    TransformComponent* transform = entity.get_component<TransformComponent>();

    const trs target(vec3(1.0f, 2.0f, 3.0f), quat(vec3(0.0f, 1.0f, 0.0f), deg2rad(30.0f)), vec3(2.0f));
    transform->set_world_transform(target);

    EXPECT_TRUE(fuzzy_equals(transform->get_world_transform(), target));
    EXPECT_TRUE(fuzzy_equals(transform->get_local_transform(), target));

    transform->set_world_position(vec3(-4.0f, 0.0f, 1.0f));
    EXPECT_TRUE(fuzzy_equals(transform->get_world_position(), vec3(-4.0f, 0.0f, 1.0f)));
    EXPECT_TRUE(fuzzy_equals(transform->get_local_transform().position, vec3(-4.0f, 0.0f, 1.0f)));

    const quat rotation(vec3(1.0f, 0.0f, 0.0f), deg2rad(45.0f));
    transform->set_world_rotation(rotation);
    EXPECT_TRUE(fuzzy_equals(transform->get_world_rotation(), rotation));

    transform->set_world_scale(vec3(3.0f));
    EXPECT_TRUE(fuzzy_equals(transform->get_world_scale(), vec3(3.0f)));

    delete world;
}

TEST_F(TransformHierarchyTest, ChildFollowsParent)
{
    const trs parent_transform(vec3(1.0f, 2.0f, 3.0f), quat(vec3(0.0f, 1.0f, 0.0f), deg2rad(90.0f)));
    transform(parent)->set_world_transform(parent_transform);
    transform(child)->set_local_transform(trs(vec3(0.0f, 0.0f, 1.0f)));

    EXPECT_TRUE(fuzzy_equals(transform(child)->get_world_position(), vec3(2.0f, 2.0f, 3.0f)));
    EXPECT_TRUE(fuzzy_equals(transform(child)->get_world_rotation(), parent_transform.rotation));

    transform(parent)->set_world_position(vec3(0.0f));

    EXPECT_TRUE(fuzzy_equals(transform(child)->get_world_position(), vec3(1.0f, 0.0f, 0.0f)));
}

TEST_F(TransformHierarchyTest, SetWorldTransformUpdatesLocal)
{
    const trs parent_transform(vec3(0.0f, 1.0f, 0.0f), quat(vec3(0.0f, 0.0f, 1.0f), deg2rad(90.0f)), vec3(2.0f));
    transform(parent)->set_world_transform(parent_transform);

    const trs child_world(vec3(5.0f, 1.0f, 0.0f), quat(vec3(0.0f, 1.0f, 0.0f), deg2rad(20.0f)), vec3(2.0f));
    transform(child)->set_world_transform(child_world);

    EXPECT_TRUE(fuzzy_equals(transform(child)->get_world_transform(), child_world));
    EXPECT_TRUE(fuzzy_equals(transform(child)->get_local_transform(), invert(parent_transform) * child_world));
    EXPECT_TRUE(fuzzy_equals(parent_transform * transform(child)->get_local_transform(), child_world));
}

TEST_F(TransformHierarchyTest, GrandchildAccumulatesChain)
{
    transform(parent)->set_world_transform(trs(vec3(1.0f, 0.0f, 0.0f), quat::identity, vec3(2.0f)));
    transform(child)->set_local_transform(trs(vec3(0.0f, 1.0f, 0.0f), quat(vec3(0.0f, 1.0f, 0.0f), deg2rad(90.0f))));
    transform(grandchild)->set_local_transform(trs(vec3(0.0f, 0.0f, 1.0f)));

    const trs expected = transform(parent)->get_world_transform() * transform(child)->get_local_transform() * transform(grandchild)->get_local_transform();

    EXPECT_TRUE(fuzzy_equals(transform(grandchild)->get_world_transform(), expected));
    EXPECT_TRUE(fuzzy_equals(transform(grandchild)->get_world_position(), vec3(3.0f, 2.0f, 0.0f)));
    EXPECT_TRUE(fuzzy_equals(transform(grandchild)->get_world_scale(), vec3(2.0f)));
}

TEST_F(TransformHierarchyTest, ReparentingKeepsLocalTransform)
{
    transform(parent)->set_world_position(vec3(10.0f, 0.0f, 0.0f));

    Entity other = world->create_entity("Other");
    transform(other)->set_world_position(vec3(0.0f, 0.0f, -10.0f));

    transform(child)->set_local_transform(trs(vec3(1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(fuzzy_equals(transform(child)->get_world_position(), vec3(11.0f, 0.0f, 0.0f)));

    child.set_parent(other.get_handle());

    EXPECT_TRUE(fuzzy_equals(transform(child)->get_local_transform().position, vec3(1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(fuzzy_equals(transform(child)->get_world_position(), vec3(1.0f, 0.0f, -10.0f)));
}
