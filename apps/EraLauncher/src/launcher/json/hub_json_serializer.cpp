#include "launcher/json/hub_json_serializer.h"
#include "launcher/core/hub_project.h"

#include <fstream>

namespace era_engine::launcher
{
	std::unordered_map<std::string, hub_project> hub_json_serializer::deserializeGlobalHubFile()
	{
		std::string filename = std::string(std::filesystem::current_path().string()) + std::string("\\Data\\EraHubMeta.ehubmeta");

		std::ifstream file(filename);

		json jprojects;
		file >> jprojects;

		int size = jprojects["Size"].get<int>();
		size_t i = 0;

		std::unordered_map<std::string, hub_project> projects;

		for (; i < size; i++)
		{
			hub_project proj;
			std::string name = jprojects["Name" + std::to_string(i)].get<std::string>();
			proj.name = name;
			proj.type = (hub_project_type)jprojects["Type" + std::to_string(i)].get<int>();
			proj.path = jprojects["Path" + std::to_string(i)].get<std::string>();
			proj.configuration = jprojects["Configuration" + std::to_string(i)].get<std::string>();
			if (!projects.count(name))
				projects.emplace(std::make_pair(name, proj));
		}

		return projects;
	}

	void hub_json_serializer::serializeGlobalHubFile(const std::unordered_map<std::string, hub_project>& projects)
	{
		json obj =
		{
			{"Size", projects.size()}
		};

		size_t i = 0;

		for (auto& proj : projects)
		{
			json jproj;

			std::string reBuiltPath = proj.second.path;
			std::string path = std::string(proj.second.name);

			jproj =
			{
				{"Name" + std::to_string(i), path.c_str()},
				{"Configuration" + std::to_string(i), proj.second.configuration},
				{"Version" + std::to_string(i), proj.second.version},
				{"Type" + std::to_string(i), proj.second.type},
				{"Path" + std::to_string(i), reBuiltPath},
			};

			obj = merge(obj, jproj);
			i++;
		}

		std::string filename = std::string(std::filesystem::current_path().string()) + std::string("\\Data\\EraHubMeta.ehubmeta");
		std::ofstream file(filename);
		file << obj << std::endl;
	}

	json hub_json_serializer::merge(const json& rhs, const json& lhs)
	{
		json result = rhs.flatten();
		json temp = lhs.flatten();

		json::iterator iter = temp.begin();
		json::iterator end = temp.end();

		for (; iter != end; ++iter)
			result[iter.key()] = iter.value();

		return result.unflatten();
	}
}