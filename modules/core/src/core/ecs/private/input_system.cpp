#include "core/ecs/private/input_system.h"
#include "core/ecs/input_root_component.h"
#include "core/imgui.h"
#include "core/log.h"
#include "core/cpu_profiling.h"

#include "ecs/update_groups.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		rttr::registration::class_<InputSystem>("InputSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr)
			.method("update", &InputSystem::update)(metadata("update_group", update_types::INPUT))
			.method("show_input", &InputSystem::show_input)(metadata("update_group", update_types::END));
	}

	InputSystem::InputSystem(World* _world)
		: System(_world)
	{
		input_rc = world->add_root_component<InputRootComponent>();
		ASSERT(input_rc != nullptr);
	}

	InputSystem::~InputSystem()
	{
	}

	void InputSystem::init()
	{
	}

	void InputSystem::update(float dt)
	{
		static bool app_focused_last_frame = true;

		CPU_PROFILE_BLOCK("Collect user input");

		UserInput& input = input_rc->frame_input;

		uint32 render_width = (uint32)ImGui::GetContentRegionAvail().x;
		uint32 render_height = (uint32)ImGui::GetContentRegionAvail().y;

		ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsItemHovered())
		{
			ImVec2 relative_mouse = ImGui::GetMousePos() - ImGui::GetItemRectMin();
			vec2 mouse_mos = { relative_mouse.x, relative_mouse.y };
			if (app_focused_last_frame)
			{
				input.mouse.dx = (int32)(mouse_mos.x - input.mouse.x);
				input.mouse.dy = (int32)(mouse_mos.y - input.mouse.y);
				input.mouse.reldx = (float)input.mouse.dx / (render_width - 1);
				input.mouse.reldy = (float)input.mouse.dy / (render_height - 1);
			}
			else
			{
				input.mouse.dx = 0;
				input.mouse.dy = 0;
				input.mouse.reldx = 0.f;
				input.mouse.reldy = 0.f;
			}
			input.mouse.x = (int32)mouse_mos.x;
			input.mouse.y = (int32)mouse_mos.y;
			input.mouse.relX = mouse_mos.x / (render_width - 1);
			input.mouse.relY = mouse_mos.y / (render_height - 1);
			input.mouse.left = { ImGui::IsMouseDown(ImGuiMouseButton_Left), ImGui::IsMouseClicked(ImGuiMouseButton_Left), ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) };
			input.mouse.right = { ImGui::IsMouseDown(ImGuiMouseButton_Right), ImGui::IsMouseClicked(ImGuiMouseButton_Right), ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right) };
			input.mouse.middle = { ImGui::IsMouseDown(ImGuiMouseButton_Middle), ImGui::IsMouseClicked(ImGuiMouseButton_Middle), ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Middle) };

			for (uint32 i = 0; i < arraysize(UserInput::keyboard); ++i)
			{
				input.keyboard[i] = { ImGui::IsKeyDown(i), ImGui::IsKeyPressed(i, false) };
			}

			input.over_window = true;
		}
		else
		{
			input.mouse.dx = 0;
			input.mouse.dy = 0;
			input.mouse.reldx = 0.f;
			input.mouse.reldy = 0.f;

			if (input.mouse.left.down && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				input.mouse.left.down = false;
			}
			if (input.mouse.right.down && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				input.mouse.right.down = false;
			}
			if (input.mouse.middle.down && !ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			{
				input.mouse.middle.down = false;
			}

			input.mouse.left.click_event = input.mouse.left.double_click_event = false;
			input.mouse.right.click_event = input.mouse.right.double_click_event = false;
			input.mouse.middle.click_event = input.mouse.middle.double_click_event = false;

			for (uint32 i = 0; i < arraysize(UserInput::keyboard); ++i)
			{
				if (!ImGui::IsKeyDown(i))
				{
					input.keyboard[i].down = false;
				}
				input.keyboard[i].press_event = false;
			}

			input.over_window = false;
		}

		vec3 current_input = vec3(
			(input.keyboard['D'].down ? 1.0f : 0.0f) + (input.keyboard['A'].down ? -1.f : 0.0f),
			(input.keyboard['E'].down ? 1.0f : 0.0f) + (input.keyboard['Q'].down ? -1.f : 0.0f),
			(input.keyboard['W'].down ? -1.0f : 0.0f) + (input.keyboard['S'].down ? 1.f : 0.0f)
		) * (input.keyboard[key_shift].down ? 3.0f : 1.f) * (input.keyboard[key_ctrl].down ? 0.1f : 1.0f);

		input_rc->last_input = input_rc->current_input;
		input_rc->desired_input = current_input;
		input_rc->current_input = current_input;

		app_focused_last_frame = ImGui::IsMousePosValid();
	}

	void InputSystem::show_input(float dt)
	{
		LOG_MESSAGE("Frame input is: X: %f Y: %f Z: %f", input_rc->current_input.x, input_rc->current_input.y, input_rc->current_input.z);
	}

}