#include "pch.h"
#include "ejson_serializer.h"
#include <scene/scene.h>
#include <fstream>

NODISCARD json merge(const json& rhs, const json& lhs)
{
	json result = rhs.flatten();
	json temp = lhs.flatten();

	json::iterator iter = temp.begin();
	json::iterator end = temp.end();

	for (; iter != end; ++iter)
		result[iter.key()] = iter.value();

	return result.unflatten();
}

void ejson_serializer::serializePrefab(eentity* entity)
{
	json obj = convert2Json(entity);
	std::string name = entity->getComponent<tag_component>().name;

	std::string filename = eproject::path + "\\assets\\prefabs\\" + name + ".eprefab";
	std::ofstream o(filename.c_str());
	o << obj << std::endl;
}

NODISCARD json ejson_serializer::convert2Json(eentity* entity)
{
	std::string name = entity->getComponent<tag_component>().name;
	entity_handle id = entity->handle;
	int childCount = 0;

	if (auto cc = entity->getComponentIfExists<child_component>())
		childCount = eentity_container::getChilds(id).size();

	json obj =
	{
		{"Name", name},
		{"Id", id},
		{"ChildCount", childCount}
	};

	return obj;
}

NODISCARD json ejson_serializer::convert2JsonWithNb(eentity* entity, int nb)
{
	std::string name = entity->getComponent<tag_component>().name;
	entity_handle id = entity->handle;
	int childCount = 0;

	if (auto cc = entity->getComponentIfExists<child_component>())
		childCount = eentity_container::getChilds(id).size();

	json obj =
	{
		{"Name" + std::to_string(nb), name},
		{"Id" + std::to_string(nb), id},
		{"ChildCount" + std::to_string(nb), childCount}
	};

	return obj;
}

NODISCARD eentity ejson_serializer::deserializePrefab(std::string_view name, escene* scene)
{
	std::string path = eproject::path + "\\assets\\prefabs\\" + name.data() + ".eprefab";

	std::ifstream file(path);
	json object;
	file >> object;

	json jname = object["Name"];
	json jid = object["Id"];
	eentity result = scene->createEntity((jname.get<std::string>() + "_prefab").c_str(), jid.get<entity_handle>());

	return result;
}

void ejson_serializer::serializeScene(escene* scene, const char* name)
{
	json result;
	int nb = 0;
	for (const auto& [handle, name] : scene->view<tag_component>().each())
	{
		eentity entity {handle, &scene->registry};
		json obj = convert2JsonWithNb(&entity, nb++);
		merge(result, obj);
	}

	json obj =
	{
		{"Count", nb}
	};

	merge(result, obj);

	std::string filename = eproject::path + "\\assets\\scenes\\" + name + ".escene";
	std::ofstream o(filename.c_str());
	o << result << std::endl;
}

NODISCARD escene ejson_serializer::deserializeScene(std::string_view name)
{
	escene scene;

	std::string path = eproject::path + "\\assets\\scenes\\" + name.data() + ".escene";

	std::ifstream file(path);
	json object;
	file >> object;

	int count = object["Count"].get<int>();
	for (int i = 0; i < count; i++)
	{

	}

	return scene;
}
