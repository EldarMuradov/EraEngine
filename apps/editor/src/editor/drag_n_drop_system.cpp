#include "editor/drag_n_drop_system.h"
#include "editor/file_utils.h"

#include <engine/engine.h>

#include <core/editor_icons.h>
#include <core/imgui.h>

#include <rendering/ecs/renderer_holder_root_component.h>

#include <ecs/update_groups.h>

#include <rttr/policy.h>
#include <rttr/registration>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui.cpp>
#include <imgui/imgui_widgets.cpp>
#include <imgui/imgui_demo.cpp>
#include <imgui/imgui_tables.cpp>
#include <imgui/imgui_internal.h>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<DragNDropSystem>("DragNDropSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr)(policy::ctor::as_raw_ptr, metadata("Tag", std::string("editor")))
			.method("update", &DragNDropSystem::update)(metadata("update_group", update_types::END)/*, metadata("After", std::vector<std::string>{"EditorToolsSystem::update"})*/);
	}

	DragNDropSystem::DragNDropSystem(World* _world)
		: System(_world)
	{

	}

	DragNDropSystem::~DragNDropSystem()
	{
	}

	void DragNDropSystem::init()
	{
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);
	}

	void DragNDropSystem::update(float dt)
	{
		float render_width = static_cast<float>(renderer_holder_rc->width);
		float render_height = static_cast<float>(renderer_holder_rc->height);

		// The drag&drop outline is rendered around the drop target. Since the image fills the frame, the outline is outside the window 
		// and thus invisible. So instead this (slightly smaller) Dummy acts as the drop target.
		// Important: This is below the input processing, so that we don't override the current element id.

		//ImGui::SetCursorPos(ImVec2(4.5f, 4.5f));
		//ImGui::Dummy(ImVec2(render_width - 9.0f, render_height - 9.0f));
		//if (ImGui::BeginDragDropTarget())
		//{
		//	if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EDITOR_ICON_MESH))
		//	{
		//		EditorFileUtils::handle_file_drop((const char*)payload->Data, world);
		//	}
		//	if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EDITOR_ICON_IMAGE_HDR))
		//	{
		//		EditorFileUtils::handle_file_drop((const char*)payload->Data, world);
		//	}
		//	ImGui::EndDragDropTarget();
		//}
	}

}