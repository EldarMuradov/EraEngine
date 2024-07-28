#pragma once

#include "launcher/render/hub_renderer.h"
#include "launcher/core/hub_project.h"

namespace era_engine::launcher
{
	struct era_hub
	{
		era_hub(ref<d3d12_hub_renderer> renderer_);
		~era_hub();

		void render();

		void removeProject(std::string name);

		void saveChanges();

		static void createProjectInFolder(hub_project& project);

		void parseProjects();

		void addProject(const hub_project& project);

		static void openProject(const hub_project& project);

		std::unordered_map<std::string, hub_project> getProjects() const noexcept { return projects; }

	private:
		ref<d3d12_hub_renderer> renderer = nullptr;

		std::unordered_map<std::string, hub_project> projects;

		std::mutex syncMutex;
	};
}