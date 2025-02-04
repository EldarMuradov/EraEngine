#include "editor/drag_n_drop_system.h"
#include "editor/file_utils.h"

#include <engine/engine.h>

#include <core/editor_icons.h>

#include <ecs/update_groups.h>

#include <rttr/policy.h>
#include <rttr/registration>

#define IMGUI_ICON_COLS 4
#define IMGUI_ICON_ROWS 4

#define IMGUI_ICON_DEFAULT_SIZE 35
#define IMGUI_ICON_DEFAULT_SPACING 3.0f

#define IMGUI_DEFINE_MATH_OPERATORS

#define MAX_NUM_IMGUI_IMAGES_PER_FRAME 128

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<DragNDropSystem>("DragNDropSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr)
			.method("update", &DragNDropSystem::update)(metadata("update_group", update_types::BEGIN));
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
	}

	void DragNDropSystem::update(float dt)
	{
		//float renderWidth = ImGui::GetContentRegionAvail().x;
		//float renderHeight = ImGui::GetContentRegionAvail().y;

		//// The drag&drop outline is rendered around the drop target. Since the image fills the frame, the outline is outside the window 
		//// and thus invisible. So instead this (slightly smaller) Dummy acts as the drop target.
		//// Important: This is below the input processing, so that we don't override the current element id.
		//ImGui::SetCursorPos(ImVec2(4.5f, 4.5f));
		//ImGui::Dummy(ImVec2(renderWidth - 9.0f, renderHeight - 9.0f));
		//
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