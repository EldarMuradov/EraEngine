#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::basic_json<>;

namespace era_engine::launcher
{
	struct hub_project;

	class hub_json_serializer final
	{
	public:
		std::unordered_map<std::string, hub_project> deserializeGlobalHubFile();
		void serializeGlobalHubFile(const std::unordered_map<std::string, hub_project>& projects);

	private:
		static json merge(const json& rhs, const json& lhs);
	};
}