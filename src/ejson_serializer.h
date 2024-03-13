#pragma once
#include <nlohmann/json.hpp>
#include "core/project.h"

using json = nlohmann::basic_json<>;

NODISCARD json merge(const json& rhs, const json& lhs);

struct escene;
struct eentity;

struct ejson_serializer
{
	template<typename T>
	static void serializeObject(T object)
	{
		json node = object;
		std::string name = eproject::path + (std::string)"\\Data\\Preferences\\" + std::string(node.type_name()) + std::string(".eobject");
		std::ofstream o(name.c_str());
		o << node << std::endl;
		o.close();
	}

	template<typename T>
	NODISCARD static T deserializeObject(std::string_view name)
	{
		std::ifstream file(eproject::path + (std::string)"\\Data\\Preferences\\" + name.data());
		json object;
		file >> object;
		file.close();

		T node = object.get<T>();

		return node;
	}

	static void serializePrefab(eentity* entity);

	NODISCARD static json convert2Json(eentity* entity);

	NODISCARD static json convert2JsonWithNb(eentity* entity, int nb);

	NODISCARD static eentity deserializePrefab(std::string_view name, escene* scene);

	static const char* serializeScene(escene* scene, const char* name);

	NODISCARD static escene deserializeScene(std::string_view name);
};