#pragma once

#include "core_api.h"

namespace era_engine
{
	enum key_code
	{
		key_shift = VK_SHIFT,
		key_ctrl = VK_CONTROL,
		key_alt = VK_MENU,
		key_esc = VK_ESCAPE,
		key_caps = VK_CAPITAL,
		key_space = VK_SPACE,
		key_enter = VK_RETURN,
		key_backspace = VK_BACK,
		key_delete = VK_DELETE,
		key_tab = VK_TAB,
		key_left = VK_LEFT,
		key_right = VK_RIGHT,
		key_up = VK_UP,
		key_down = VK_DOWN,
		key_print = VK_SNAPSHOT,
		key_f1 = VK_F1,
		key_f2 = VK_F2,
		key_f3 = VK_F3,
		key_f4 = VK_F4,
		key_f5 = VK_F5,
		key_f6 = VK_F6,
		key_f7 = VK_F7,
		key_f8 = VK_F8,
		key_f9 = VK_F9,
		key_f10 = VK_F10,
		key_f11 = VK_F11,
		key_f12 = VK_F12,
	};

	struct ERA_CORE_API InputKey
	{
		bool down;
		bool press_event;
	};

	struct ERA_CORE_API InputMouseButton
	{
		bool down;
		bool click_event;
		bool double_click_event;
		bool padding;
	};

	struct ERA_CORE_API MouseInput
	{
		InputMouseButton left;
		InputMouseButton right;
		InputMouseButton middle;
		float scroll;

		int32 x;
		int32 y;
		int32 dx;
		int32 dy;

		float relX;
		float relY;
		float reldx;
		float reldy;
	};

	struct ERA_CORE_API UserInput
	{
		InputKey keyboard[128];
		MouseInput mouse;
		bool over_window;
		bool padding[3];
	};
}