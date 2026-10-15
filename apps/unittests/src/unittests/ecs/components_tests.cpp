#include <gtest/gtest.h>

#include <ecs/world.h>
#include <ecs/base_components/base_components.h>
#include <core/ecs/tags_component.h>

TEST(ECS_Components, TagsComponent) {

	using namespace era_engine;

	World* runtime_world = new World(World::GAMEPLAY_WORLD_NAME);
	runtime_world->init();

	Entity entity = runtime_world->create_entity("Entity");
	TagsComponent* tags_component = entity.add_component<TagsComponent>();

	tags_component->add_tag("test");
	tags_component->add_tag("test_2");

	EXPECT_TRUE(tags_component->size() == 2);

	EXPECT_TRUE(tags_component->has_tag("test"));

	EXPECT_FALSE(tags_component->has_tag("test_fail"));

	EXPECT_TRUE(tags_component->remove_tag("test_2"));

	EXPECT_TRUE(tags_component->size() == 1);

	delete runtime_world;
}

TEST(ECS_Components, ChildComponent) {

	using namespace era_engine;

	World* runtime_world = new World(World::GAMEPLAY_WORLD_NAME);
	runtime_world->init();

	Entity parent = runtime_world->create_entity();

	{
		Entity child = runtime_world->create_entity();
		child.set_parent(parent.get_handle());

		EXPECT_TRUE(EntityContainer::get_childs(runtime_world, parent.get_handle()).size() == 1);

		child.set_parent(runtime_world->get_root_entity().get_handle());

		runtime_world->destroy_entity(child);
	}

	EXPECT_TRUE(EntityContainer::get_childs(runtime_world, parent.get_handle()).size() == 0);

	delete runtime_world;

}

TEST(ECS_Components, TagsComponentDeduplicatesAndSorts)
{
	using namespace era_engine;

	World* world = new World("GameWorld");
	world->init();

	Entity entity = world->create_entity();
	TagsComponent* tags_component = entity.add_component<TagsComponent>();

	tags_component->add_tag("b");
	tags_component->add_tag("a");
	tags_component->add_tag("b");
	tags_component->add_tag("c");

	EXPECT_EQ(tags_component->size(), 3u);

	const std::vector<std::string>& tags = tags_component->get_container().get_tags();
	ASSERT_EQ(tags.size(), 3u);
	EXPECT_EQ(tags[0], "a");
	EXPECT_EQ(tags[1], "b");
	EXPECT_EQ(tags[2], "c");

	EXPECT_FALSE(tags_component->remove_tag("d"));

	tags_component->clear();

	EXPECT_EQ(tags_component->size(), 0u);
	EXPECT_FALSE(tags_component->has_tag("a"));

	delete world;
}

TEST(ECS_Components, TagsContainer)
{
	using namespace era_engine;

	TagsContainer container;

	EXPECT_EQ(container.size(), 0u);
	EXPECT_FALSE(container.has_tag("tag"));
	EXPECT_FALSE(container.remove_tag("tag"));

	container.add_tag("tag");
	container.add_tag("tag");
	container.add_tag("another");

	EXPECT_TRUE(container.has_tag("tag"));
	EXPECT_TRUE(container.has_tag("another"));
	EXPECT_EQ(container.size(), 2u);

	EXPECT_TRUE(container.remove_tag("tag"));
	EXPECT_FALSE(container.has_tag("tag"));
	EXPECT_EQ(container.size(), 1u);

	container.add_tag("tag");
	EXPECT_TRUE(container.has_tag("tag"));
	EXPECT_EQ(container.size(), 2u);
}

TEST(ECS_Components, NameComponent)
{
	using namespace era_engine;

	World* world = new World("GameWorld");
	world->init();

	Entity named = world->create_entity("Named");
	Entity unnamed = world->create_entity();

	EXPECT_STREQ(named.get_component<NameComponent>()->name, "Named");
	EXPECT_EQ(unnamed.get_component_if_exists<NameComponent>(), nullptr);

	NameComponent* name_component = unnamed.add_component<NameComponent>("Late");
	EXPECT_STREQ(name_component->name, "Late");

	NameComponent* same_component = named.add_component<NameComponent>("Renamed");
	EXPECT_STREQ(same_component->name, "Named");

	delete world;
}
