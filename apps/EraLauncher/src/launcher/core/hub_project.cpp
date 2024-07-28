#include "launcher/core/hub_project.h"
#include "launcher/core/d3d12_hub.h"

#include <imgui/imgui.h>

namespace  era_engine::launcher
{
	void renderProject(const hub_project& project)
	{
		ImGui::Separator();
		ImGui::Text(project.name.c_str());
		ImGui::Text(project.path.c_str());
		if (ImGui::Button("Open Project"))
		{
			era_hub::openProject(project);
		}
		ImGui::Separator();
	}
}
