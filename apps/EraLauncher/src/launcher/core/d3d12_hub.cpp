#include "launcher/core/d3d12_hub.h"
#include "launcher/json/hub_json_serializer.h"
#include "launcher/build/project_builder.h"

namespace era_engine::launcher
{
	era_hub::era_hub(ref<d3d12_hub_renderer> renderer_)
	{
		renderer = renderer_;
	}

	era_hub::~era_hub()
	{
	}

	void era_hub::render()
	{
		renderer->beginFrame();

		renderer->render();

		renderer->endFrame();
	}

	void era_hub::removeProject(std::string name)
	{
		projects.erase(name);
	}

	void era_hub::saveChanges()
	{
		lock lock{ syncMutex };
		hub_json_serializer serializer;
		serializer.serializeGlobalHubFile(projects);
	}

	void era_hub::createProjectInFolder(hub_project& project)
	{
		builder builder;
		builder.build(project);
	}

	void era_hub::parseProjects()
	{
		lock lock{syncMutex};
		hub_json_serializer serializer;
		projects = serializer.deserializeGlobalHubFile();
	}

	void era_hub::addProject(const hub_project& project)
	{
		projects.emplace(std::make_pair(project.name, project));
	}

	void era_hub::openProject(const hub_project& project)
	{
	}

}