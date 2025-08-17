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