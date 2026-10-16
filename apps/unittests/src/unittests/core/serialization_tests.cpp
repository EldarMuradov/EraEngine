#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <core/serialization/binary_serializer.h>
#include <core/bounding_volumes.h>

namespace
{
	using namespace era_engine;

	enum class SerializationTestEnum : uint16
	{
		FIRST = 1,
		SECOND = 512
	};

	struct SerializationTestItem
	{
		int32 id = 0;
		std::string name;
		vec3 position = vec3(0.0f);

		bool operator==(const SerializationTestItem& other) const
		{
			return id == other.id && name == other.name && position == other.position;
		}

		ERA_BINARY_SERIALIZE(id, name, position)
	};

	struct SerializationTestContainer
	{
		std::vector<SerializationTestItem> items;
		std::map<std::string, int32> lookup;
		std::optional<float> weight;
		SerializationTestEnum type = SerializationTestEnum::FIRST;
		bool enabled = false;

		ERA_BINARY_SERIALIZE(items, lookup, weight, type, enabled)
	};

	template <typename T>
	T round_trip(const T& source, size_t* out_size = nullptr)
	{
		const BinaryDataArchive archive = BinarySerializer::serialize(source);
		EXPECT_EQ(archive.size(), BinarySerializer::get_size(source));

		T result{};
		const size_t read = BinarySerializer::deserialize(BinaryData(archive), result);
		EXPECT_EQ(read, archive.size());

		if (out_size != nullptr)
		{
			*out_size = archive.size();
		}

		return result;
	}
}

TEST(Core_Serialization, PodTypes)
{
	size_t size = 0;

	EXPECT_EQ(round_trip(int32(-123456), &size), -123456);
	EXPECT_EQ(size, sizeof(int32));

	EXPECT_EQ(round_trip(uint64(0xDEADBEEFCAFEull), &size), 0xDEADBEEFCAFEull);
	EXPECT_EQ(size, sizeof(uint64));

	EXPECT_FLOAT_EQ(round_trip(3.25f), 3.25f);
	EXPECT_DOUBLE_EQ(round_trip(-1.0e10), -1.0e10);

	EXPECT_TRUE(round_trip(true, &size));
	EXPECT_EQ(size, 1u);
	EXPECT_FALSE(round_trip(false));

	EXPECT_EQ(round_trip(SerializationTestEnum::SECOND, &size), SerializationTestEnum::SECOND);
	EXPECT_EQ(size, sizeof(uint16));
}

TEST(Core_Serialization, MathTypes)
{
	EXPECT_EQ(round_trip(vec2(1.0f, -2.0f)), vec2(1.0f, -2.0f));
	EXPECT_EQ(round_trip(vec3(1.0f, -2.0f, 3.5f)), vec3(1.0f, -2.0f, 3.5f));
	EXPECT_EQ(round_trip(vec4(1.0f, 2.0f, 3.0f, 4.0f)), vec4(1.0f, 2.0f, 3.0f, 4.0f));

	const quat rotation(normalize(vec3(1.0f, 2.0f, 3.0f)), 0.7f);
	EXPECT_EQ(round_trip(rotation), rotation);

	const trs transform(vec3(1.0f, 2.0f, 3.0f), rotation, vec3(0.5f, 2.0f, 1.0f));
	size_t size = 0;
	EXPECT_EQ(round_trip(transform, &size), transform);
	EXPECT_EQ(size, sizeof(vec3) * 2 + sizeof(quat));

	mat4 matrix = mat4::identity;
	matrix.m03 = 5.0f;
	matrix.m21 = -2.0f;
	const mat4 matrix_result = round_trip(matrix);
	EXPECT_EQ(std::memcmp(matrix_result.m, matrix.m, sizeof(matrix.m)), 0);

	const mat3 rotation_matrix = quaternion_to_mat3(rotation);
	const mat3 rotation_matrix_result = round_trip(rotation_matrix);
	EXPECT_EQ(std::memcmp(rotation_matrix_result.m, rotation_matrix.m, sizeof(rotation_matrix.m)), 0);
}

TEST(Core_Serialization, Strings)
{
	size_t size = 0;

	EXPECT_EQ(round_trip(std::string("Era Engine"), &size), "Era Engine");
	EXPECT_EQ(size, sizeof(uint32) + 10u);

	EXPECT_EQ(round_trip(std::string(), &size), "");
	EXPECT_EQ(size, sizeof(uint32));

	const std::string binary_string("a\0b\0c", 5);
	EXPECT_EQ(round_trip(binary_string), binary_string);
}

TEST(Core_Serialization, SequentialContainers)
{
	const std::vector<int32> numbers = { 1, -2, 3, 1000000 };
	size_t size = 0;
	EXPECT_EQ(round_trip(numbers, &size), numbers);
	EXPECT_EQ(size, sizeof(uint32) + numbers.size() * sizeof(int32));

	EXPECT_TRUE(round_trip(std::vector<int32>()).empty());

	const std::vector<std::string> strings = { "first", "", "third" };
	EXPECT_EQ(round_trip(strings), strings);

	const std::vector<std::vector<float>> nested = { { 1.0f, 2.0f }, {}, { 3.0f } };
	EXPECT_EQ(round_trip(nested), nested);

	const std::list<int32> list = { 5, 4, 3 };
	EXPECT_EQ(round_trip(list), list);

	const std::array<uint8, 4> array = { 1, 2, 3, 4 };
	EXPECT_EQ(round_trip(array, &size), array);
	EXPECT_EQ(size, 4u);
}

TEST(Core_Serialization, AssociativeContainers)
{
	const std::map<std::string, int32> map = { { "a", 1 }, { "bb", 2 }, { "ccc", 3 } };
	EXPECT_EQ(round_trip(map), map);

	const std::unordered_map<int32, std::string> unordered_map = { { 1, "one" }, { 2, "two" } };
	EXPECT_EQ(round_trip(unordered_map), unordered_map);

	const std::set<int32> set = { 9, 3, 7 };
	EXPECT_EQ(round_trip(set), set);

	const std::unordered_set<std::string> unordered_set = { "x", "y" };
	EXPECT_EQ(round_trip(unordered_set), unordered_set);

	const std::pair<int32, std::string> pair = { 4, "four" };
	EXPECT_EQ(round_trip(pair), pair);
}

TEST(Core_Serialization, OptionalAndPointers)
{
	size_t size = 0;

	const std::optional<int32> value = 17;
	EXPECT_EQ(round_trip(value, &size), value);
	EXPECT_EQ(size, 1u + sizeof(int32));

	const std::optional<int32> empty_value;
	EXPECT_FALSE(round_trip(empty_value, &size).has_value());
	EXPECT_EQ(size, 1u);

	const std::shared_ptr<std::string> shared = std::make_shared<std::string>("shared");
	const std::shared_ptr<std::string> shared_result = round_trip(shared);
	ASSERT_NE(shared_result, nullptr);
	EXPECT_NE(shared_result, shared);
	EXPECT_EQ(*shared_result, "shared");

	EXPECT_EQ(round_trip(std::shared_ptr<int32>()), nullptr);

	std::unique_ptr<vec3> unique = std::make_unique<vec3>(1.0f, 2.0f, 3.0f);
	const BinaryDataArchive archive = BinarySerializer::serialize(unique);

	std::unique_ptr<vec3> unique_result;
	BinarySerializer::deserialize(BinaryData(archive), unique_result);
	ASSERT_NE(unique_result, nullptr);
	EXPECT_EQ(*unique_result, *unique);
}

TEST(Core_Serialization, UserStructs)
{
	SerializationTestContainer source;
	source.items.push_back({ 1, "first", vec3(1.0f, 0.0f, 0.0f) });
	source.items.push_back({ 2, "second", vec3(0.0f, 2.0f, 0.0f) });
	source.lookup = { { "first", 1 }, { "second", 2 } };
	source.weight = 0.5f;
	source.type = SerializationTestEnum::SECOND;
	source.enabled = true;

	const SerializationTestContainer result = round_trip(source);

	EXPECT_EQ(result.items, source.items);
	EXPECT_EQ(result.lookup, source.lookup);
	EXPECT_EQ(result.weight, source.weight);
	EXPECT_EQ(result.type, source.type);
	EXPECT_EQ(result.enabled, source.enabled);
}

TEST(Core_Serialization, MultipleValues)
{
	const int32 number = 42;
	const std::string text = "text";
	const vec3 position(1.0f, 2.0f, 3.0f);

	const size_t size = BinarySerializer::get_size(number, text, position);
	EXPECT_EQ(size, sizeof(int32) + sizeof(uint32) + text.size() + sizeof(vec3));

	std::vector<uint8> buffer(size);
	EXPECT_EQ(BinarySerializer::serialize(buffer.data(), number, text, position), size);

	int32 number_result = 0;
	std::string text_result;
	vec3 position_result(0.0f);
	EXPECT_EQ(BinarySerializer::deserialize(buffer.data(), buffer.size(), number_result, text_result, position_result), size);

	EXPECT_EQ(number_result, number);
	EXPECT_EQ(text_result, text);
	EXPECT_EQ(position_result, position);
}

TEST(Core_Serialization, EngineStructs)
{
	const BoundingBox box = BoundingBox::from_min_max(vec3(-1.0f), vec3(2.0f));
	const BoundingBox box_result = round_trip(box);
	EXPECT_EQ(box_result.min_corner, box.min_corner);
	EXPECT_EQ(box_result.max_corner, box.max_corner);

	BoundingSphere sphere;
	sphere.center = vec3(1.0f, 2.0f, 3.0f);
	sphere.radius = 4.0f;
	const BoundingSphere sphere_result = round_trip(sphere);
	EXPECT_EQ(sphere_result.center, sphere.center);
	EXPECT_FLOAT_EQ(sphere_result.radius, sphere.radius);

	const std::vector<IndexedTriangle16> triangles = { { 0, 1, 2 }, { 2, 3, 0 } };
	const std::vector<IndexedTriangle16> triangles_result = round_trip(triangles);
	ASSERT_EQ(triangles_result.size(), 2u);
	EXPECT_EQ(triangles_result[1].a, 2);
	EXPECT_EQ(triangles_result[1].b, 3);
	EXPECT_EQ(triangles_result[1].c, 0);
}

TEST(Core_Serialization, BinaryData)
{
	const uint8 bytes[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	const BinaryDataArchive source(bytes, sizeof(bytes));

	EXPECT_EQ(source.size(), sizeof(bytes));
	EXPECT_EQ(source, BinaryDataArchive(bytes, sizeof(bytes)));
	EXPECT_NE(source, BinaryDataArchive(4, uint8(1)));

	const BinaryDataArchive archive = BinarySerializer::serialize(source);
	EXPECT_EQ(archive.size(), sizeof(uint32) + sizeof(bytes));

	BinaryDataArchive archive_result;
	BinarySerializer::deserialize(BinaryData(archive), archive_result);
	EXPECT_EQ(archive_result, source);

	BinaryData view;
	const size_t read = BinarySerializer::deserialize(BinaryData(archive), view);
	EXPECT_EQ(read, archive.size());
	ASSERT_EQ(view.size(), sizeof(bytes));
	EXPECT_EQ(view.data(), archive.raw_data() + sizeof(uint32));
	EXPECT_EQ(std::memcmp(view.data(), bytes, sizeof(bytes)), 0);

	BinaryData truncated_view;
	BinarySerializer::deserialize(archive.raw_data(), sizeof(uint32) + 3, truncated_view);
	EXPECT_EQ(truncated_view.size(), 3u);

	const std::string text = "abc";
	EXPECT_EQ(BinaryData(text).size(), 3u);
}

TEST(Core_Serialization, BinaryDataArchive)
{
	BinaryDataArchive archive(4, uint8(7));
	EXPECT_EQ(archive.data(), std::vector<uint8>(4, 7));

	archive.resize(6);
	EXPECT_EQ(archive.size(), 6u);

	const uint8 bytes[] = { 9, 8, 7 };
	archive.fill(bytes, sizeof(bytes));
	EXPECT_EQ(archive.data(), std::vector<uint8>({ 9, 8, 7 }));

	archive.reset();
	EXPECT_EQ(archive.size(), 0u);
}

TEST(Core_Serialization, DeserializeDoesNotReadPastBuffer)
{
	const vec3 source(1.0f, 2.0f, 3.0f);
	const BinaryDataArchive archive = BinarySerializer::serialize(source);

	vec3 result(-1.0f);
	BinarySerializer::deserialize(archive.raw_data(), archive.size() - 1, result);
	EXPECT_EQ(result, vec3(-1.0f));

	int32 number = 5;
	BinarySerializer::deserialize(archive.raw_data(), 2, number);
	EXPECT_EQ(number, 5);
}
