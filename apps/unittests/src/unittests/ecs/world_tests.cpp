#include <gtest/gtest.h>

#include <ecs/base_components/base_components.h>
#include <ecs/world.h>

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