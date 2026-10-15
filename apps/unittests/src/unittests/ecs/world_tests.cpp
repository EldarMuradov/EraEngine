#include <gtest/gtest.h>

#include <ecs/base_components/base_components.h>
#include <ecs/world.h>
#include <core/ecs/tags_component.h>

#include <algorithm>

namespace
{
	using namespace era_engine;

	class WorldTestComponent final : public Component
	{
	public:
		WorldTestComponent() = default;
		WorldTestComponent(ref<Entity::EcsData> _data, int32 _value = 0)
			: Component(_data), value(_value)
		{
		}

		int32 value = 0;
	};

	class WorldOtherTestComponent final : public Component
	{
	public:
		WorldOtherTestComponent() = default;
		WorldOtherTestComponent(ref<Entity::EcsData> _data)
			: Component(_data)
		{
		}
	};

	struct WorldTestContext
	{
		WorldTestContext() = default;
		WorldTestContext(int32 _value) : value(_value) {}

		int32 value = 0;
	};

	bool contains_handle(const std::vector<Entity::Handle>& handles, Entity::Handle handle)
	{
		return std::find(handles.begin(), handles.end(), handle) != handles.end();
	}
}

TEST(ECS_World, Initialization) {

	using namespace era_engine;

	World* runtime_world = new World("GameWorld");
	runtime_world->init();

	ASSERT_TRUE(runtime_world->size() == 1);

	delete runtime_world;
}

TEST(ECS_World, AddEntity) {

	using namespace era_engine;

	World* runtime_world = new World("GameWorld");
	runtime_world->init();

	const char* entity_name = "EntityName";

	Entity entity = runtime_world->create_entity(entity_name);

	ASSERT_TRUE(runtime_world->size() == 2);
	ASSERT_TRUE(entity.is_valid());

	const NameComponent* name_component = entity.get_component_if_exists<NameComponent>();

	ASSERT_TRUE(name_component != nullptr);
	ASSERT_TRUE(strcmp(name_component->name, entity_name) == 0);

	runtime_world->destroy_entity(entity);

	ASSERT_TRUE(runtime_world->size() == 1);

	delete runtime_world;

}

TEST(ECS_World, RootEntityHasBaseComponents)
{
	World* world = new World("GameWorld");
	world->init();

	Entity root = world->get_root_entity();

	ASSERT_TRUE(root.is_valid());
	EXPECT_EQ(root.get_world(), world);
	EXPECT_TRUE(root.get_parent_handle() == Entity::NullHandle);
	EXPECT_TRUE(root.has_component<TransformComponent>());
	EXPECT_TRUE(root.has_component<TagsComponent>());

	const NameComponent* name_component = root.get_component_if_exists<NameComponent>();
	ASSERT_NE(name_component, nullptr);
	EXPECT_STREQ(name_component->name, "RootEntity");

	delete world;
}

TEST(ECS_World, CreatedEntityIsChildOfRoot)
{
	World* world = new World("GameWorld");
	world->init();

	Entity entity = world->create_entity();
	const Entity root = world->get_root_entity();

	ASSERT_TRUE(entity.is_valid());
	EXPECT_EQ(entity.get_world(), world);
	EXPECT_EQ(entity.get_parent_handle(), root.get_handle());
	EXPECT_TRUE(entity.has_component<TransformComponent>());
	EXPECT_FALSE(entity.has_component<NameComponent>());
	EXPECT_TRUE(contains_handle(EntityContainer::get_childs(world, root.get_handle()), entity.get_handle()));

	delete world;
}

TEST(ECS_World, CreatedEntitiesHaveUniqueHandles)
{
	World* world = new World("GameWorld");
	world->init();

	std::vector<Entity::Handle> handles;
	for (int32 i = 0; i < 64; ++i)
	{
		handles.push_back(world->create_entity().get_handle());
	}

	std::sort(handles.begin(), handles.end());
	EXPECT_EQ(std::unique(handles.begin(), handles.end()), handles.end());
	EXPECT_EQ(world->size(), 65u);

	delete world;
}

TEST(ECS_World, GetEntityByHandle)
{
	World* world = new World("GameWorld");
	world->init();

	Entity entity = world->create_entity("Entity");
	const Entity::Handle handle = entity.get_handle();

	Entity found = world->get_entity(handle);
	ASSERT_TRUE(found.is_valid());
	EXPECT_EQ(found, entity);
	EXPECT_TRUE(found == handle);

	world->destroy_entity(handle);

	EXPECT_FALSE(world->get_entity(handle).is_valid());

	delete world;
}

TEST(ECS_World, CreateEntityWithExplicitHandle)
{
	World* world = new World("GameWorld");
	world->init();

	const Entity::Handle handle = static_cast<Entity::Handle>(42);

	Entity entity = world->create_entity(handle, "Explicit");
	ASSERT_TRUE(entity.is_valid());
	EXPECT_EQ(entity.get_handle(), handle);
	EXPECT_STREQ(entity.get_component<NameComponent>()->name, "Explicit");
	EXPECT_EQ(world->size(), 2u);

	Entity same = world->create_entity(handle);
	EXPECT_EQ(same, entity);
	EXPECT_EQ(world->size(), 2u);

	delete world;
}

TEST(ECS_World, TryCreateEntityInPlace)
{
	World* world = new World("GameWorld");
	world->init();

	Entity invalid_place;
	EXPECT_FALSE(world->try_create_entity_in_place(invalid_place, "Entity").is_valid());

	Entity place = world->create_entity("Place");
	Entity created = world->try_create_entity_in_place(place, "Entity");
	EXPECT_EQ(created, place);
	EXPECT_EQ(world->size(), 2u);

	delete world;
}

TEST(ECS_World, DestroyNullHandleIsNoop)
{
	World* world = new World("GameWorld");
	world->init();

	world->create_entity();
	world->destroy_entity(static_cast<Entity::Handle>(Entity::NullHandle));

	EXPECT_EQ(world->size(), 2u);

	delete world;
}

TEST(ECS_World, DestroyEntityDestroysChilds)
{
	World* world = new World("GameWorld");
	world->init();

	Entity parent = world->create_entity("Parent");
	Entity child = world->create_entity("Child");
	Entity grandchild = world->create_entity("Grandchild");
	Entity unrelated = world->create_entity("Unrelated");

	child.set_parent(parent.get_handle());
	grandchild.set_parent(child.get_handle());

	const Entity::Handle parent_handle = parent.get_handle();
	const Entity::Handle child_handle = child.get_handle();
	const Entity::Handle grandchild_handle = grandchild.get_handle();

	world->destroy_entity(parent_handle);

	EXPECT_EQ(world->size(), 2u);
	EXPECT_FALSE(world->get_entity(parent_handle).is_valid());
	EXPECT_FALSE(world->get_entity(child_handle).is_valid());
	EXPECT_FALSE(world->get_entity(grandchild_handle).is_valid());
	EXPECT_TRUE(world->get_entity(unrelated.get_handle()).is_valid());
	EXPECT_TRUE(EntityContainer::get_childs(world, parent_handle).empty());
	EXPECT_TRUE(EntityContainer::get_childs(world, child_handle).empty());
	EXPECT_FALSE(contains_handle(EntityContainer::get_childs(world, world->get_root_entity().get_handle()), parent_handle));

	delete world;
}

TEST(ECS_World, DestroyEntityKeepsChildsOnRequest)
{
	World* world = new World("GameWorld");
	world->init();

	Entity parent = world->create_entity("Parent");
	Entity child = world->create_entity("Child");
	child.set_parent(parent.get_handle());

	const Entity::Handle parent_handle = parent.get_handle();

	world->destroy_entity(parent_handle, false);

	const Entity::Handle root_handle = world->get_root_entity().get_handle();

	EXPECT_FALSE(world->get_entity(parent_handle).is_valid());
	EXPECT_TRUE(world->get_entity(child.get_handle()).is_valid());
	EXPECT_EQ(world->size(), 2u);
	EXPECT_EQ(child.get_parent_handle(), root_handle);
	EXPECT_TRUE(contains_handle(EntityContainer::get_childs(world, root_handle), child.get_handle()));
	EXPECT_TRUE(EntityContainer::get_childs(world, parent_handle).empty());

	delete world;
}

TEST(ECS_World, DestroyedChildIsRemovedFromParent)
{
	World* world = new World("GameWorld");
	world->init();

	Entity parent = world->create_entity("Parent");
	Entity child = world->create_entity("Child");
	child.set_parent(parent.get_handle());

	const Entity::Handle parent_handle = parent.get_handle();
	const Entity::Handle child_handle = child.get_handle();

	ASSERT_EQ(EntityContainer::get_childs(world, parent_handle).size(), 1u);

	world->destroy_entity(child_handle);

	EXPECT_TRUE(EntityContainer::get_childs(world, parent_handle).empty());

	delete world;
}

TEST(ECS_World, HierarchyIsIsolatedBetweenWorlds)
{
	World* first_world = new World("GameWorld");
	first_world->init();

	Entity first_parent = first_world->create_entity("Parent");
	Entity first_child = first_world->create_entity("Child");
	first_child.set_parent(first_parent.get_handle());

	World* second_world = new World("GameWorld");
	second_world->init();

	Entity second_entity = second_world->create_entity("Entity");

	EXPECT_TRUE(EntityContainer::get_childs(second_world, second_entity.get_handle()).empty());
	EXPECT_EQ(EntityContainer::get_childs(second_world, second_world->get_root_entity().get_handle()).size(), 1u);
	EXPECT_EQ(EntityContainer::get_childs(first_world, first_parent.get_handle()).size(), 1u);

	delete second_world;
	delete first_world;
}

TEST(ECS_World, Tags)
{
	World* world = new World("GameWorld");
	world->init();

	EXPECT_FALSE(world->has_tag("physics"));

	world->add_tag("physics");
	world->add_tag("render");
	world->add_tag("physics");

	EXPECT_TRUE(world->has_tag("physics"));
	EXPECT_TRUE(world->has_tag("render"));
	EXPECT_FALSE(world->has_tag("editor"));
	EXPECT_EQ(world->get_tags_container().size(), 2u);
	EXPECT_EQ(world->get_root_component<TagsComponent>()->size(), 2u);

	EXPECT_TRUE(world->remove_tag("physics"));
	EXPECT_FALSE(world->remove_tag("physics"));
	EXPECT_FALSE(world->has_tag("physics"));
	EXPECT_TRUE(world->has_tag("render"));

	world->clear_tags();

	EXPECT_FALSE(world->has_tag("render"));
	EXPECT_EQ(world->get_tags_container().size(), 0u);

	delete world;
}

TEST(ECS_World, RootComponents)
{
	World* world = new World("GameWorld");
	world->init();

	EXPECT_EQ(world->get_root_component<WorldTestComponent>(), nullptr);

	WorldTestComponent* component = world->add_root_component<WorldTestComponent>(5);
	ASSERT_NE(component, nullptr);
	EXPECT_EQ(component->value, 5);
	EXPECT_EQ(component->get_handle(), world->get_root_entity().get_handle());

	WorldTestComponent* same_component = world->add_root_component<WorldTestComponent>(10);
	EXPECT_EQ(same_component, component);
	EXPECT_EQ(same_component->value, 5);
	EXPECT_EQ(world->get_root_component<WorldTestComponent>(), component);

	delete world;
}

TEST(ECS_World, DestroyUnknownHandleIsNoop)
{
	World* world = new World("GameWorld");
	world->init();

	Entity entity = world->create_entity();
	const Entity::Handle handle = entity.get_handle();

	world->destroy_entity(handle);
	world->destroy_entity(handle);
	world->destroy_entity(static_cast<Entity::Handle>(1000));

	EXPECT_EQ(world->size(), 1u);

	delete world;
}

TEST(ECS_World, ContextVariables)
{
	World* world = new World("GameWorld");
	world->init();

	EXPECT_FALSE(world->does_context_variable_exist<WorldTestContext>());

	WorldTestContext& context = world->create_or_get_context_variable<WorldTestContext>(7);
	EXPECT_EQ(context.value, 7);
	EXPECT_TRUE(world->does_context_variable_exist<WorldTestContext>());

	WorldTestContext& same_context = world->create_or_get_context_variable<WorldTestContext>(9);
	EXPECT_EQ(&same_context, &context);
	EXPECT_EQ(same_context.value, 7);

	world->get_context_variable<WorldTestContext>().value = 11;
	EXPECT_EQ(context.value, 11);

	world->delete_context_variable<WorldTestContext>();
	EXPECT_FALSE(world->does_context_variable_exist<WorldTestContext>());

	delete world;
}

TEST(ECS_World, ViewIteratesMatchingEntities)
{
	World* world = new World("GameWorld");
	world->init();

	for (int32 i = 0; i < 5; ++i)
	{
		Entity entity = world->create_entity();
		entity.add_component<WorldTestComponent>(i);
		if (i % 2 == 0)
		{
			entity.add_component<WorldOtherTestComponent>();
		}
	}
	world->create_entity();

	EXPECT_EQ(world->number_of_components_of_type<WorldTestComponent>(), 5u);
	EXPECT_EQ(world->number_of_components_of_type<WorldOtherTestComponent>(), 3u);
	EXPECT_EQ(world->number_of_components_of_type<TransformComponent>(), 7u);

	int32 sum = 0;
	uint32 count = 0;
	for (auto [handle, test_component, other_component] : world->view<WorldTestComponent, WorldOtherTestComponent>().each())
	{
		EXPECT_EQ(test_component.get_handle(), handle);
		sum += test_component.value;
		++count;
	}

	EXPECT_EQ(count, 3u);
	EXPECT_EQ(sum, 0 + 2 + 4);

	delete world;
}

TEST(ECS_World, NonOwningGroups)
{
	World* world = new World("GameWorld");
	world->init();

	for (int32 i = 0; i < 6; ++i)
	{
		Entity entity = world->create_entity();
		entity.add_component<WorldTestComponent>(i);
		if (i < 2)
		{
			entity.add_component<WorldOtherTestComponent>();
		}
	}

	auto with_both = world->group(components_group<WorldTestComponent, WorldOtherTestComponent>);
	EXPECT_EQ(with_both.size(), 2u);

	auto without_other = world->group(components_group<WorldTestComponent>, components_group<WorldOtherTestComponent>);
	EXPECT_EQ(without_other.size(), 4u);

	for (Entity::Handle handle : without_other)
	{
		EXPECT_GE(world->get_entity(handle).get_component<WorldTestComponent>()->value, 2);
	}

	delete world;
}

TEST(ECS_World, RegisteredByName)
{
	const std::string name = "UnitTestRegisteredWorld";

	World* world = new World(name);
	world->init();

	EXPECT_EQ(get_world_by_name(name), world);
	EXPECT_NE(get_worlds().find(name), get_worlds().end());

	delete world;

	EXPECT_EQ(get_worlds().find(name), get_worlds().end());
	EXPECT_EQ(get_world_by_name(name), nullptr);
	EXPECT_EQ(get_worlds().find(name), get_worlds().end());
}

TEST(ECS_World, SameNameWorldsKeepFirstRegistered)
{
	const std::string name = "UnitTestSameNameWorld";

	World* first = new World(name);
	World* second = new World(name);

	EXPECT_EQ(get_world_by_name(name), first);

	delete second;

	EXPECT_EQ(get_world_by_name(name), first);

	delete first;

	EXPECT_EQ(get_world_by_name(name), nullptr);
}
