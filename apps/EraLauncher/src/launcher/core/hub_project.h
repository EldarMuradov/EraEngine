#pragma once

namespace era_engine::launcher
{
	enum class hub_project_type : uint8_t
	{
		default_3D = 0,
		default_3D_Demo,
		default_2D
	};

	struct hub_project
	{
		std::string path = "";
		std::string name = "";
		std::string version = "0.01";
		std::string configuration = "";

		hub_project_type type = hub_project_type::default_3D_Demo;
	};

	void renderProject(const hub_project& project);
}