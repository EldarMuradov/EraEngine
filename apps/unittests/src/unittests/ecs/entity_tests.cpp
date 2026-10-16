#include <gtest/gtest.h>

#include <ecs/world.h>
#include <ecs/component.h>
#include <ecs/base_components/base_components.h>

#include <cstring>
#include <string>

namespace
{
	using namespace era_engine;

	class EntityTestComponent final : public Component
	{
	public:
		EntityTestComponent() = default;
		EntityTestComponent(ref<Entity::EcsData> _data, int32 _number = 0, float _scale = 1.0f)
			: Component(_data), number(_number), scale(_scale)
		{
		}

		int32 number = 0;
		float scale = 1.0f;
	};

	class EntityOtherTestComponent final : public Component
	{
	public:
		EntityOtherTestComponent() = default;
		EntityOtherTestComponent(ref<Entity::EcsData> _data)
			: Component(_data)
		{
		}
	};
}

TEST(ECS_Entity, DefaultEntityIsInvalid)
{
	Entity entity;

	EXPECT_FALSE(entity.is_valid());
	EXPECT_FALSE(Entity::null.is_valid());
	EXPECT_EQ(entity, Entity::null);
}

TEST(ECS_Entity, CopiesShareIdentity)
{
	World* world = new World("GameWorld");
	world->init();

	Entity entity = world->create_entity("Entity");
	Entity other = world->create_entity("Other");

	Entity copy = entity;
	EXPECT_EQ(copy, entity);
	EXPECT_TRUE(copy == entity.get_handle());
	EXPECT_NE(copy, other);

	Entity assigned;
	assigned = other;
	EXPECT_EQ(assigned, other);

	Entity moved = std::move(copy);
	EXPECT_EQ(moved, entity);
	EXPECT_TRUE(moved.is_valid());

	delete world;
}

TEST(ECS_Entity, AddComponentForwardsArguments)
{
	World* world = new World("GameWorld");
	world->init();

	Entity entity = world->create_entity();

	EntityTestComponent* component = entity.add_component<EntityTestComponent>(3, 0.5f);

	ASSERT_NE(component, nullptr);
	EXPECT_EQ(component->number, 3);
	EXPECT_FLOAT_EQ(component->scale, 0.5f);
	EXPECT_EQ(component->get_world(), world);
	EXPECT_EQ(component->get_handle(), entity.get_handle());
	EXPECT_EQ(component->get_entity(), entity);
	EXPECT_TRUE(entity.has_component<EntityTestComponent>());
	EXPECT_EQ(entity.get_component<EntityTestComponent>(), component);
	EXPECT_EQ(entity.get_component_if_exists<EntityTestComponent>(), component);

	delete world;
}

TEST(ECS_Entity, AddExistingComponentReturnsExisting)
{
	World* world = new World("GameWorld");
	world->init();

	Entity entity = world->create_entity();

	EntityTestComponent* component = entity.add_component<EntityTestComponent>(1);
	EntityTestComponent* same_component = entity.add_component<EntityTestComponent>(2);

	EXPECT_EQ(same_component, component);
	EXPECT_EQ(component->number, 1);
	EXPECT_EQ(world->number_of_components_of_type<EntityTestComponent>(), 1u);

	delete world;
}

TEST(ECS_Entity, RemoveComponent)
{
	World* world = new World("GameWorld");
	world->init();

	Entity entity = world->create_entity();
	entity.add_component<EntityTestComponent>();
	entity.add_component<EntityOtherTestComponent>();

	entity.remove_component<EntityTestComponent>();

	EXPECT_FALSE(entity.has_component<EntityTestComponent>());
	EXPECT_EQ(entity.get_component_if_exists<EntityTestComponent>(), nullptr);
	EXPECT_TRUE(entity.has_component<EntityOtherTestComponent>());
	EXPECT_TRUE(entity.has_component<TransformComponent>());

	entity.remove_component<EntityTestComponent>();
	EXPECT_FALSE(entity.has_component<EntityTestComponent>());

	delete world;
}

TEST(ECS_Entity, ComponentsAreIndependentPerEntity)
{
	World* world = new World("GameWorld");
	world->init();

	Entity first = world->create_entity();
	Entity second = world->create_entity();

	first.add_component<EntityTestComponent>(1);
	second.add_component<EntityTestComponent>(2);

	const Entity& const_first = first;
	EXPECT_EQ(const_first.get_component<EntityTestComponent>()->number, 1);
	EXPECT_EQ(second.get_component<EntityTestComponent>()->number, 2);

	first.remove_component<EntityTestComponent>();

	EXPECT_FALSE(first.has_component<EntityTestComponent>());
	EXPECT_EQ(second.get_component<EntityTestComponent>()->number, 2);

	delete world;
}

TEST(ECS_Entity, DestroyingEntityDestroysComponents)
{
	World* world = new World("GameWorld");
	world->init();

	Entity first = world->create_entity();
	Entity second = world->create_entity();
	first.add_component<EntityTestComponent>(1);
	second.add_component<EntityTestComponent>(2);

	world->destroy_entity(first);

	EXPECT_EQ(world->number_of_components_of_type<EntityTestComponent>(), 1u);
	EXPECT_EQ(second.get_component<EntityTestComponent>()->number, 2);

	delete world;
}

TEST(ECS_Entity, EntityPtrTracksLifetime)
{
	World* world = new World("GameWorld");
	world->init();

	EntityPtr ptr;
	EXPECT_TRUE(ptr.is_empty());

	Entity::Handle handle = Entity::null_handle;
	{
		Entity entity = world->create_entity("Entity");
		entity.add_component<EntityTestComponent>();
		handle = entity.get_handle();

		ptr = EntityPtr(entity);

		EXPECT_FALSE(ptr.is_empty());
		EXPECT_EQ(ptr.get(), entity);
		EXPECT_EQ(ptr, EntityPtr(entity.get_data_weakref()));
	}

	world->destroy_entity(handle);

	EXPECT_TRUE(ptr.is_empty());
	EXPECT_FALSE(ptr.get().is_valid());

	delete world;
}

TEST(ECS_Entity, ComponentPtr)
{
	World* world = new World("GameWorld");
	world->init();

	ComponentPtr empty_ptr;
	EXPECT_TRUE(empty_ptr.is_empty());
	EXPECT_FALSE(empty_ptr.get_entity().is_valid());

	Entity entity = world->create_entity();
	EntityTestComponent* component = entity.add_component<EntityTestComponent>();

	ComponentPtr ptr(component);
	EXPECT_FALSE(ptr.is_empty());
	EXPECT_EQ(ptr.get(), component);
	EXPECT_EQ(ptr.get_for_write(), component);
	EXPECT_EQ(ptr.get_entity(), entity);

	delete world;
}

TEST(ECS_Entity, NameComponentTruncatesLongNames)
{
	World* world = new World("GameWorld");
	world->init();

	const std::string long_name(64, 'a');
	Entity entity = world->create_entity(long_name.c_str());

	const NameComponent* name_component = entity.get_component<NameComponent>();
	EXPECT_EQ(std::strlen(name_component->name), sizeof(name_component->name) - 1);
	EXPECT_EQ(std::string(name_component->name), long_name.substr(0, sizeof(name_component->name) - 1));

	delete world;
}

TEST(ECS_Entity, SetParent)
{
	World* world = new World("GameWorld");
	world->init();

	Entity parent = world->create_entity("Parent");
	Entity first_child = world->create_entity("FirstChild");
	Entity second_child = world->create_entity("SecondChild");

	first_child.set_parent(parent.get_handle());
	second_child.set_parent(parent.get_handle());

	EXPECT_EQ(first_child.get_parent_handle(), parent.get_handle());

	const std::vector<Entity> childs = EntityContainer::get_entity_childs(world, parent.get_handle());
	ASSERT_EQ(childs.size(), 2u);
	EXPECT_EQ(childs[0], first_child);
	EXPECT_EQ(childs[1], second_child);

	first_child.set_parent(second_child.get_handle());

	EXPECT_EQ(EntityContainer::get_childs(world, parent.get_handle()).size(), 1u);
	EXPECT_EQ(EntityContainer::get_childs(world, second_child.get_handle()).size(), 1u);
	EXPECT_EQ(first_child.get_parent_handle(), second_child.get_handle());

	first_child.set_parent(world->get_root_entity().get_handle());
	second_child.set_parent(world->get_root_entity().get_handle());

	EXPECT_TRUE(EntityContainer::get_childs(world, parent.get_handle()).empty());
	EXPECT_TRUE(EntityContainer::get_childs(world, second_child.get_handle()).empty());

	delete world;
}
