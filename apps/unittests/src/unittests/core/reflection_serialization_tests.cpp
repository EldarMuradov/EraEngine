#include <gtest/gtest.h>

#include <ecs/world.h>
#include <ecs/observable_member.h>
#include <ecs/serialization/component_registry.h>
#include <ecs/serialization/reflection_serializer.h>
#include <ecs/serialization/serialization_meta.h>
#include <ecs/serialization/value_adapters.h>

#include <rttr/registration>

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace
{
	using namespace era_engine;

	enum class ReflectionTestColor : uint8
	{
		RED,
		GREEN,
		BLUE
	};

	enum class ReflectionTestFlags : uint32
	{
		NONE = 0,
		FIRST = 1 << 0,
		SECOND = 1 << 1
	};

	struct ReflectionTestFlagsHolder
	{
		ReflectionTestFlags flags = ReflectionTestFlags::NONE;
		ObservableMember<ReflectionTestFlags> observable_flags = ReflectionTestFlags::NONE;
	};

	struct ReflectionTestNested
	{
		float weight = 0.0f;
		std::string label;

		bool operator==(const ReflectionTestNested& other) const = default;
	};

	class ReflectionTestObject
	{
	public:
		int32 get_private_value() const { return private_value; }
		void set_private_value(int32 value) { private_value = value; }

		bool flag = false;
		int8 tiny = 0;
		uint8 byte = 0;
		int32 number = 0;
		uint64 big = 0;
		float value = 0.0f;
		double precise = 0.0;
		std::string text;
		vec2 v2 = vec2(0.0f);
		vec3 v3 = vec3(0.0f);
		vec4 v4 = vec4(0.0f);
		quat rotation = quat::identity;
		trs transform = trs::identity;
		ReflectionTestColor color = ReflectionTestColor::RED;
		ReflectionTestNested nested;
		std::vector<int32> numbers;
		std::vector<ReflectionTestNested> nested_items;
		std::array<float, 3> fixed = { 0.0f, 0.0f, 0.0f };
		std::optional<uint32> maybe;
		std::optional<uint32> empty_maybe;
		ObservableMember<float> observable;
		ObservableMember<vec3> observable_vector;
		float hidden = 0.0f;

	private:
		int32 private_value = 0;

		RTTR_REGISTRATION_FRIEND
	};

	class ReflectionTestComponent final : public Component
	{
	public:
		ReflectionTestComponent() = default;
		ReflectionTestComponent(ref<Entity::EcsData> _data) : Component(_data) {}

		float speed = 1.0f;

		ERA_VIRTUAL_REFLECT(Component)
	};

	class ReflectionRuntimeComponent final : public Component
	{
	public:
		ReflectionRuntimeComponent() = default;
		ReflectionRuntimeComponent(ref<Entity::EcsData> _data) : Component(_data) {}

		ERA_VIRTUAL_REFLECT(Component)
	};

	ReflectionTestObject make_filled_object()
	{
		ReflectionTestObject object;
		object.flag = true;
		object.tiny = -7;
		object.byte = 200;
		object.number = -123456;
		object.big = 1ull << 40;
		object.value = 0.1f;
		object.precise = 1.0 / 3.0;
		object.text = "hello: world";
		object.v2 = vec2(1.0f, 2.0f);
		object.v3 = vec3(1.0f, 2.0f, 3.0f);
		object.v4 = vec4(1.0f, 2.0f, 3.0f, 4.0f);
		object.rotation = normalize(quat(0.1f, 0.2f, 0.3f, 0.9f));
		object.transform = trs(vec3(4.0f, 5.0f, 6.0f), normalize(quat(0.3f, 0.0f, 0.0f, 1.0f)), vec3(2.0f, 2.0f, 2.0f));
		object.color = ReflectionTestColor::BLUE;
		object.nested = { 0.5f, "nested" };
		object.numbers = { 1, 2, 3 };
		object.nested_items = { { 1.0f, "a" }, { 2.0f, "b" } };
		object.fixed = { 7.0f, 8.0f, 9.0f };
		object.maybe = 42u;
		object.observable = 3.5f;
		object.observable_vector = vec3(1.0f, 0.0f, 1.0f);
		object.hidden = 5.0f;
		object.set_private_value(77);
		return object;
	}
}

RTTR_REGISTRATION
{
	using namespace rttr;

	registration::enumeration<ReflectionTestColor>("ReflectionTestColor")
		(
			value("RED", ReflectionTestColor::RED),
			value("GREEN", ReflectionTestColor::GREEN),
			value("BLUE", ReflectionTestColor::BLUE)
		);

	registration::enumeration<ReflectionTestFlags>("ReflectionTestFlags")
		(
			value("NONE", ReflectionTestFlags::NONE),
			value("FIRST", ReflectionTestFlags::FIRST),
			value("SECOND", ReflectionTestFlags::SECOND)
		);
	ValueAdapters::register_enum_converter<ReflectionTestFlags>();
	ValueAdapters::register_observable<ReflectionTestFlags>();

	registration::class_<ReflectionTestFlagsHolder>("ReflectionTestFlagsHolder")
		.property("flags", &ReflectionTestFlagsHolder::flags)
		.property("observable_flags", &ReflectionTestFlagsHolder::observable_flags);

	registration::class_<ReflectionTestNested>("ReflectionTestNested")
		.property("weight", &ReflectionTestNested::weight)
		.property("label", &ReflectionTestNested::label);

	registration::class_<ReflectionTestObject>("ReflectionTestObject")
		.property("flag", &ReflectionTestObject::flag)
		.property("tiny", &ReflectionTestObject::tiny)
		.property("byte", &ReflectionTestObject::byte)
		.property("number", &ReflectionTestObject::number)
		.property("big", &ReflectionTestObject::big)
		.property("value", &ReflectionTestObject::value)
		.property("precise", &ReflectionTestObject::precise)
		.property("text", &ReflectionTestObject::text)
		.property("v2", &ReflectionTestObject::v2)
		.property("v3", &ReflectionTestObject::v3)
		.property("v4", &ReflectionTestObject::v4)
		.property("rotation", &ReflectionTestObject::rotation)
		.property("transform", &ReflectionTestObject::transform)
		.property("color", &ReflectionTestObject::color)
		.property("nested", &ReflectionTestObject::nested)
		.property("numbers", &ReflectionTestObject::numbers)
		.property("nested_items", &ReflectionTestObject::nested_items)
		.property("fixed", &ReflectionTestObject::fixed)
		.property("maybe", &ReflectionTestObject::maybe)
		.property("empty_maybe", &ReflectionTestObject::empty_maybe)
		.property("observable", &ReflectionTestObject::observable)
		.property("observable_vector", &ReflectionTestObject::observable_vector)
		.property("hidden", &ReflectionTestObject::hidden)(metadata(SerializationMeta::NOT_SERIALIZABLE_PROPERTY, true))
		.property("private_value", &ReflectionTestObject::get_private_value, &ReflectionTestObject::set_private_value);

	registration::class_<ReflectionTestComponent>("ReflectionTestComponent")
		.constructor<ref<Entity::EcsData>>()
		.property("speed", &ReflectionTestComponent::speed);
	ComponentRegistry::register_component<ReflectionTestComponent>();

	registration::class_<ReflectionRuntimeComponent>("ReflectionRuntimeComponent")(metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
		.constructor<ref<Entity::EcsData>>();
	ComponentRegistry::register_component<ReflectionRuntimeComponent>();
}

TEST(Core_ReflectionSerialization, RoundTripPreservesValues)
{
	const ReflectionTestObject source = make_filled_object();
	const std::string text = ReflectionSerializer::serialize(source);

	ReflectionTestObject loaded;
	ASSERT_TRUE(ReflectionSerializer::deserialize(text, loaded));

	EXPECT_EQ(loaded.flag, source.flag);
	EXPECT_EQ(loaded.tiny, source.tiny);
	EXPECT_EQ(loaded.byte, source.byte);
	EXPECT_EQ(loaded.number, source.number);
	EXPECT_EQ(loaded.big, source.big);
	EXPECT_EQ(loaded.value, source.value);
	EXPECT_EQ(loaded.precise, source.precise);
	EXPECT_EQ(loaded.text, source.text);
	EXPECT_EQ(loaded.v2, source.v2);
	EXPECT_EQ(loaded.v3, source.v3);
	EXPECT_EQ(loaded.v4, source.v4);
	EXPECT_EQ(loaded.rotation, source.rotation);
	EXPECT_EQ(loaded.transform, source.transform);
	EXPECT_EQ(loaded.color, source.color);
	EXPECT_EQ(loaded.nested, source.nested);
	EXPECT_EQ(loaded.numbers, source.numbers);
	EXPECT_EQ(loaded.nested_items, source.nested_items);
	EXPECT_EQ(loaded.fixed, source.fixed);
	EXPECT_EQ(loaded.maybe, source.maybe);
	EXPECT_EQ(loaded.empty_maybe, source.empty_maybe);
	EXPECT_EQ(loaded.observable.get(), source.observable.get());
	EXPECT_EQ(loaded.observable_vector.get(), source.observable_vector.get());
	EXPECT_EQ(loaded.get_private_value(), source.get_private_value());
}

TEST(Core_ReflectionSerialization, EnumIsWrittenByName)
{
	const ReflectionTestObject object = make_filled_object();
	const std::string text = ReflectionSerializer::serialize(object);

	EXPECT_NE(text.find("color: BLUE"), std::string::npos);
}

TEST(Core_ReflectionSerialization, NoSerializePropertyIsSkipped)
{
	const ReflectionTestObject object = make_filled_object();
	const std::string text = ReflectionSerializer::serialize(object);
	EXPECT_EQ(text.find("hidden"), std::string::npos);

	ReflectionTestObject loaded;
	ReflectionSerializer::deserialize("hidden: 3.0", loaded);
	EXPECT_EQ(loaded.hidden, 0.0f);
}

TEST(Core_ReflectionSerialization, UnknownAndMissingPropertiesKeepDefaults)
{
	ReflectionTestObject loaded;
	loaded.number = 5;

	ASSERT_TRUE(ReflectionSerializer::deserialize("unknown_property: 1\nvalue: 2.5", loaded));

	EXPECT_EQ(loaded.number, 5);
	EXPECT_EQ(loaded.value, 2.5f);
}

TEST(Core_ReflectionSerialization, InvalidValueKeepsDefault)
{
	ReflectionTestObject loaded;
	loaded.number = 5;
	loaded.color = ReflectionTestColor::GREEN;

	ASSERT_TRUE(ReflectionSerializer::deserialize("number: not_a_number\ncolor: PURPLE\nvalue: 1.5", loaded));

	EXPECT_EQ(loaded.number, 5);
	EXPECT_EQ(loaded.color, ReflectionTestColor::GREEN);
	EXPECT_EQ(loaded.value, 1.5f);
}

TEST(Core_ReflectionSerialization, ReadObservableMemberIsChanged)
{
	ReflectionTestObject loaded;
	loaded.observable.sync_changes();

	ASSERT_TRUE(ReflectionSerializer::deserialize("observable: 4.0", loaded));

	EXPECT_EQ(loaded.observable.get(), 4.0f);
	EXPECT_TRUE(loaded.observable.is_changed());
}

TEST(Core_ReflectionSerialization, NullClearsOptional)
{
	ReflectionTestObject loaded;
	loaded.maybe = 3u;

	ASSERT_TRUE(ReflectionSerializer::deserialize("maybe: ~", loaded));

	EXPECT_FALSE(loaded.maybe.has_value());
}

TEST(Core_ReflectionSerialization, FlagCombinationsRoundTrip)
{
	ReflectionTestFlagsHolder source;
	source.flags = static_cast<ReflectionTestFlags>(3);
	source.observable_flags = ReflectionTestFlags::SECOND;

	const std::string text = ReflectionSerializer::serialize(source);
	EXPECT_NE(text.find("flags: 3"), std::string::npos);
	EXPECT_NE(text.find("observable_flags: SECOND"), std::string::npos);

	ReflectionTestFlagsHolder loaded;
	ASSERT_TRUE(ReflectionSerializer::deserialize(text, loaded));
	EXPECT_EQ(loaded.flags, source.flags);
	EXPECT_EQ(loaded.observable_flags.get(), ReflectionTestFlags::SECOND);
}

TEST(Core_ReflectionSerialization, MalformedYamlFails)
{
	ReflectionTestObject loaded;

	EXPECT_FALSE(ReflectionSerializer::deserialize("value: [1, 2", loaded));
}

TEST(Core_ComponentRegistry, FindsRegisteredComponent)
{
	const ComponentTypeInfo* by_name = ComponentRegistry::find("ReflectionTestComponent");
	const ComponentTypeInfo* by_type = ComponentRegistry::find(rttr::type::get<ReflectionTestComponent>());
	const ComponentTypeInfo* by_storage = ComponentRegistry::find_by_storage_id(entt::type_hash<ReflectionTestComponent>::value());

	ASSERT_NE(by_name, nullptr);
	EXPECT_EQ(by_name, by_type);
	EXPECT_EQ(by_name, by_storage);
	EXPECT_TRUE(by_name->is_serializable());
	EXPECT_EQ(ComponentRegistry::find("UnknownComponent"), nullptr);
}

TEST(Core_ComponentRegistry, NotSerializableComponentIsMarked)
{
	const ComponentTypeInfo* info = ComponentRegistry::find("ReflectionRuntimeComponent");

	ASSERT_NE(info, nullptr);
	EXPECT_FALSE(info->is_serializable());
}

TEST(Core_ComponentRegistry, AddsGetsAndRemovesByType)
{
	World* world = new World("ReflectionRegistryWorld");
	world->init();

	Entity entity = world->create_entity();
	const ComponentTypeInfo* info = ComponentRegistry::find("ReflectionTestComponent");
	ASSERT_NE(info, nullptr);

	EXPECT_EQ(info->get(entity), nullptr);

	Component* component = info->add(entity);
	ASSERT_NE(component, nullptr);
	EXPECT_EQ(component->get_type(), rttr::type::get<ReflectionTestComponent>());
	EXPECT_EQ(component, entity.get_component_if_exists<ReflectionTestComponent>());
	EXPECT_EQ(info->get(entity), component);

	info->remove(entity);
	EXPECT_FALSE(entity.has_component<ReflectionTestComponent>());

	delete world;
}
