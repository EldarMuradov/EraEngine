#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::basic_json<>;

json merge(const json& rhs, const json& lhs);

struct escene;
struct eentity;

struct ejson_serializer 
{
	template<typename T>
	static void serializeObject(T object)
	{
		json node = object;
		std::string name = Project::ProjectPath + (std::string)"\\data\\Preferences\\" + std::string(node.type_name()) + std::string(".eobject");
		std::ofstream o(name.c_str());
		o << node << std::endl;
	}

	template<typename T>
	static T deserializeObject(std::string_view name)
	{
		std::ifstream file(Project::ProjectPath + (std::string)"\\data\\Preferences\\" + name.data());
		json object;
		file >> object;

		T node = object.get<T>();

		return node;
	}

	static void serializePrefab(eentity* entity);

	static json convert2Json(eentity* entity);

	static json convert2JsonWithNb(eentity* entity, int nb);

	static eentity* deserializePrefab(std::string_view name, escene* scene);

	static void serializeScene(escene* scene, const char* name);

	static escene deserializeScene(std::string_view name);
};