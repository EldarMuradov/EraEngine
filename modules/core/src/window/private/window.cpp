// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "window/window.h"
#include "window/software_window.h"

#include "core/imgui.h"
#include "core/string.h"
#include "core/log.h"
#include "core/input.h"

#include "asset/image.h"

#include <DirectXTex/DirectXTex.h>

#include <Windowsx.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <algorithm>

namespace era_engine
{
	class Win32InputHandler
	{
	private:
		HWND hwnd;

		struct RawMouseInput
		{
			float dx = 0.0f;
			float dy = 0.0f;
			bool updated = false;
		};

		float accumulated_raw_dx = 0.0f;
		float accumulated_raw_dy = 0.0f;
		bool raw_input_available = false;

		RawMouseInput raw_mouse_input;

		UserInput current_input;
		UserInput previous_input;

		MouseMode current_mouse_mode = MouseMode::VISIBLE;
		MouseMode previous_mouse_mode = MouseMode::VISIBLE;
		bool cursor_visible = true;
		bool cursor_captured = false;

		POINT capture_position;
		POINT last_mouse_position;
		RECT window_rect;

		struct ClickTracker
		{
			uint32 click_count = 0;
			float last_click_time = 0.0f;
		} click_trackers[5];

		float cursor_show_timer = 0.0f;
		float cursor_show_duration = 2.0f;
		bool mouse_moved_while_captured = false;

		float idle_timer = 0.0f;

		LARGE_INTEGER perf_frequency;
		LARGE_INTEGER perf_counter;

		InputConfig config;

	public:
		Win32InputHandler() : hwnd(nullptr)
		{
			QueryPerformanceFrequency(&perf_frequency);
			QueryPerformanceCounter(&perf_counter);
			ZeroMemory(&current_input, sizeof(UserInput));
			ZeroMemory(&previous_input, sizeof(UserInput));
			current_input.mouse.visible = true;
		}

		~Win32InputHandler()
		{
			release_mouse();
		}

		void initialize(HWND window_handle)
		{
			hwnd = window_handle;
			register_raw_input();
		}

		void set_config(const InputConfig& new_config)
		{
			config = new_config;
		}

		const InputConfig& get_config() const
		{
			return config;
		}

		void set_mouse_mode(MouseMode mode)
		{
			if (current_mouse_mode == mode)
			{
				return;
			}

			previous_mouse_mode = current_mouse_mode;
			current_mouse_mode = mode;

			apply_mouse_mode();
		}

		MouseMode get_mouse_mode() const
		{
			return current_mouse_mode;
		}

		void toggle_mouse_capture()
		{
			if (current_mouse_mode == MouseMode::VISIBLE || current_mouse_mode == MouseMode::HIDDEN)
			{
				set_mouse_mode(config.use_raw_input ? MouseMode::CAPTURED_RAW : MouseMode::CAPTURED);
			}
			else
			{
				set_mouse_mode(MouseMode::VISIBLE);
			}
		}

		void toggle_cursor_visibility()
		{
			if (cursor_visible)
			{
				hide_cursor();
			}
			else
			{
				show_cursor();
			}
		}

		void show_cursor()
		{
			if (!cursor_visible)
			{
				cursor_visible = true;
				while (ShowCursor(TRUE) < 0)
				{
					std::this_thread::yield();
				}
				current_input.mouse.visible = true;
			}
		}

		void hide_cursor()
		{
			if (cursor_visible)
			{
				cursor_visible = false;
				while (ShowCursor(FALSE) >= 0)
				{
					std::this_thread::yield();
				}
				current_input.mouse.visible = false;
			}
		}

		void capture_mouse()
		{
			if (!cursor_captured && hwnd)
			{
				cursor_captured = true;

				GetCursorPos(&last_mouse_position);

				GetWindowRect(hwnd, &window_rect);
				capture_position.x = (window_rect.left + window_rect.right) / 2;
				capture_position.y = (window_rect.top + window_rect.bottom) / 2;

				SetCursorPos(capture_position.x, capture_position.y);

				SetCapture(hwnd);

				current_input.mouse.captured = true;
			}
			else
			{
				release_mouse();
			}
		}

		void release_mouse()
		{
			if (cursor_captured)
			{
				cursor_captured = false;
				ReleaseCapture();

				if (current_mouse_mode == MouseMode::VISIBLE)
				{
					SetCursorPos(last_mouse_position.x, last_mouse_position.y);
				}

				current_input.mouse.captured = false;
				current_input.mouse.raw_dx = 0;
				current_input.mouse.raw_dy = 0;
			}
		}

		void temporarily_show_cursor(float duration = 2.0f)
		{
			if (cursor_captured)
			{
				show_cursor();
				release_mouse();
				cursor_show_timer = duration;
				cursor_show_duration = duration;
				mouse_moved_while_captured = false;
				GetCursorPos(&last_mouse_position);
			}
		}

		bool is_mouse_captured() const
		{
			return cursor_captured;
		}

		bool is_cursor_visible() const 
		{
			return cursor_visible;
		}

		void process_message(UINT msg, WPARAM w_param, LPARAM l_param)
		{
			switch (msg)
			{
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				handle_key_down(w_param, l_param);
				break;

			case WM_KEYUP:
			case WM_SYSKEYUP:
				handle_key_up(w_param, l_param);
				break;

			case WM_MOUSEMOVE:
				handle_mouse_move(w_param, l_param);
				break;

			case WM_LBUTTONDOWN:
				handle_mouse_button_down(0, w_param, l_param);
				break;

			case WM_LBUTTONUP:
				handle_mouse_button_up(0, w_param, l_param);
				break;

			case WM_RBUTTONDOWN:
				handle_mouse_button_down(1, w_param, l_param);
				break;

			case WM_RBUTTONUP:
				handle_mouse_button_up(1, w_param, l_param);
				break;

			case WM_MBUTTONDOWN:
				handle_mouse_button_down(2, w_param, l_param);
				break;

			case WM_MBUTTONUP:
				handle_mouse_button_up(2, w_param, l_param);
				break;

			case WM_XBUTTONDOWN:
				handle_mouse_button_down(HIWORD(w_param) == XBUTTON1 ? 3 : 4, w_param, l_param);
				break;

			case WM_XBUTTONUP:
				handle_mouse_button_up(HIWORD(w_param) == XBUTTON1 ? 3 : 4, w_param, l_param);
				break;

			case WM_MOUSEWHEEL:
				handle_mouse_wheel(w_param, l_param);
				break;

			case WM_INPUT:
				handle_raw_input(l_param);
				break;

			case WM_SETFOCUS:
				handle_focus_gained();
				break;

			case WM_KILLFOCUS:
				handle_focus_lost();
				break;

			case WM_ACTIVATE:
				if (LOWORD(w_param) == WA_INACTIVE)
				{
					handle_focus_lost();
				}
				else
				{
					handle_focus_gained();
				}
				break;

			case WM_MOUSELEAVE:
				current_input.mouse.visible = false;
				current_input.over_window = false;
				break;

			case WM_MOUSEHOVER:
				current_input.over_window = true;
				break;
			}
		}

		void begin_frame(float dt)
		{
			previous_input = current_input;

			for (int i = 0; i < KEY_COUNT; i++)
			{
				current_input.keyboard[i].press_event = false;
				current_input.keyboard[i].release_event = false;
				if (current_input.keyboard[i].down)
				{
					current_input.keyboard[i].held_time += dt;
				}
			}

			clear_mouse_events(&current_input.mouse.left, dt);
			clear_mouse_events(&current_input.mouse.right, dt);
			clear_mouse_events(&current_input.mouse.middle, dt);
			clear_mouse_events(&current_input.mouse.x1, dt);
			clear_mouse_events(&current_input.mouse.x2, dt);

			current_input.mouse.dx = 0;
			current_input.mouse.dy = 0;
			current_input.mouse.scroll_delta = 0;
			current_input.mouse.raw_dx = 0;
			current_input.mouse.raw_dy = 0;
			current_input.mouse.reldx = 0;
			current_input.mouse.reldy = 0;

			current_input.any_key_pressed = false;
		}

		void update(float dt)
		{
			if (cursor_show_timer > 0.0f)
			{
				cursor_show_timer -= dt;

				POINT current_pos;
				GetCursorPos(&current_pos);

				if (current_pos.x != last_mouse_position.x || current_pos.y != last_mouse_position.y)
				{
					mouse_moved_while_captured = true;
					last_mouse_position = current_pos;
				}

				if (cursor_show_timer <= 0.0f || mouse_moved_while_captured)
				{
					hide_cursor();
					capture_mouse();
					cursor_show_timer = 0.0f;
					mouse_moved_while_captured = false;
				}
			}

			handle_idle_detection(dt);

			process_input_toggles();

			consume_raw_input();
		}

		const UserInput& get_input() const
		{
			return current_input;
		}

		bool is_key_down(uint32 key) const
		{
			if (key < KEY_COUNT)
			{
				return current_input.keyboard[key].down;
			}
			return false;
		}

		bool is_key_pressed(uint32 key) const
		{
			if (key < KEY_COUNT)
			{
				return current_input.keyboard[key].press_event;
			}
			return false;
		}

		bool is_key_released(uint32 key) const
		{
			if (key < KEY_COUNT)
			{
				return current_input.keyboard[key].release_event;
			}
			return false;
		}

		float get_key_held_time(uint32 key) const
		{
			if (key < KEY_COUNT)
			{
				return current_input.keyboard[key].held_time;
			}
			return 0.0f;
		}

		bool is_mouse_down(uint32 button) const
		{
			switch (button)
			{
			case 0: return current_input.mouse.left.down;
			case 1: return current_input.mouse.right.down;
			case 2: return current_input.mouse.middle.down;
			case 3: return current_input.mouse.x1.down;
			case 4: return current_input.mouse.x2.down;
			default: return false;
			}
		}

		bool is_mouse_clicked(uint32 button) const
		{
			switch (button)
			{
			case 0: return current_input.mouse.left.click_event;
			case 1: return current_input.mouse.right.click_event;
			case 2: return current_input.mouse.middle.click_event;
			case 3: return current_input.mouse.x1.click_event;
			case 4: return current_input.mouse.x2.click_event;
			default: return false;
			}
		}

		bool is_mouse_double_clicked(uint32 button) const
		{
			switch (button)
			{
			case 0: return current_input.mouse.left.double_click_event;
			case 1: return current_input.mouse.right.double_click_event;
			case 2: return current_input.mouse.middle.double_click_event;
			case 3: return current_input.mouse.x1.double_click_event;
			case 4: return current_input.mouse.x2.double_click_event;
			default: return false;
			}
		}

	private:
		void register_raw_input()
		{
			if (!hwnd)
			{
				return;
			}

			RAWINPUTDEVICE rid[1];
			rid[0].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
			rid[0].usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
			rid[0].dwFlags = RIDEV_INPUTSINK;   // Receive input even when not in foreground
			rid[0].hwndTarget = hwnd;

			if (!RegisterRawInputDevices(rid, 1, sizeof(rid[0])))
			{
				DWORD error = GetLastError();
				LOG_ERROR("Failed to register raw input device\n");
			}
		}

		void apply_mouse_mode()
		{
			switch (current_mouse_mode)
			{
			case MouseMode::VISIBLE:
				release_mouse();
				show_cursor();
				break;

			case MouseMode::HIDDEN:
				release_mouse();
				hide_cursor();
				break;

			case MouseMode::CAPTURED:
				hide_cursor();
				capture_mouse();
				break;

			case MouseMode::CAPTURED_RAW:
				hide_cursor();
				capture_mouse();
				break;
			}
		}

		void handle_key_down(WPARAM w_param, LPARAM l_param)
		{
			uint32 key = (uint32)w_param;
			if (key < KEY_COUNT)
			{
				if (!current_input.keyboard[key].down)
				{
					current_input.keyboard[key].press_event = true;
					current_input.keyboard[key].held_time = 0.0f;
				}
				current_input.keyboard[key].down = true;
				current_input.any_key_pressed = true;
			}
		}

		void handle_key_up(WPARAM w_param, LPARAM l_param)
		{
			uint32 key = (uint32)w_param;
			if (key < KEY_COUNT)
			{
				current_input.keyboard[key].down = false;
				current_input.keyboard[key].release_event = true;
			}
		}

		void handle_mouse_move(WPARAM w_param, LPARAM l_param)
		{
			int x = GET_X_LPARAM(l_param);
			int y = GET_Y_LPARAM(l_param);

			current_input.mouse.dx = x - current_input.mouse.x;
			current_input.mouse.dy = y - current_input.mouse.y;
			current_input.mouse.x = x;
			current_input.mouse.y = y;

			RECT client_rect;
			GetClientRect(hwnd, &client_rect);
			int width = client_rect.right - client_rect.left;
			int height = client_rect.bottom - client_rect.top;

			if (width > 0 && height > 0)
			{
				current_input.mouse.rel_x = (float)x / width;
				current_input.mouse.rel_y = (float)y / height;
				current_input.mouse.reldx = (float)current_input.mouse.dx / width;
				current_input.mouse.reldy = (float)current_input.mouse.dy / height;
			}

			current_input.over_window = true;

			if (cursor_captured)
			{
				SetCursorPos(capture_position.x, capture_position.y);
			}
		}

		void handle_mouse_button_down(int button, WPARAM w_param, LPARAM l_param)
		{
			InputMouseButton* mouse_button = get_mouse_button(button);
			if (!mouse_button)
			{
				return;
			}

			mouse_button->down = true;
			mouse_button->click_event = true;
			mouse_button->held_time = 0.0f;

			float current_time = get_current_time();
			if (current_time - click_trackers[button].last_click_time < 0.3f)
			{
				click_trackers[button].click_count++;
				if (click_trackers[button].click_count >= 2)
				{
					mouse_button->double_click_event = true;
					click_trackers[button].click_count = 0;
				}
			}
			else
			{
				click_trackers[button].click_count = 1;
			}
			click_trackers[button].last_click_time = current_time;
			mouse_button->click_count = click_trackers[button].click_count;

			if (button == 1 && config.right_click_to_capture)
			{
				if (!cursor_captured)
				{
					toggle_mouse_capture();
				}
			}
		}

		void handle_mouse_button_up(int button, WPARAM w_param, LPARAM l_param)
		{
			InputMouseButton* mouse_button = get_mouse_button(button);
			if (!mouse_button)
			{
				return;
			}

			mouse_button->down = false;
			mouse_button->release_event = true;
		}

		void handle_mouse_wheel(WPARAM w_param, LPARAM l_param)
		{
			float delta = (float)GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA;
			current_input.mouse.scroll += delta;
			current_input.mouse.scroll_delta = delta;
		}

		void consume_raw_input()
		{
			raw_mouse_input.dx = 0.0f;
			raw_mouse_input.dy = 0.0f;
			raw_mouse_input.updated = false;
		}

		bool has_raw_input() const
		{
			return raw_mouse_input.updated;
		}

		float get_raw_dx() const { return raw_mouse_input.dx; }
		float get_raw_dy() const { return raw_mouse_input.dy; }

		void handle_raw_input(LPARAM l_param)
		{
			UINT dw_size = 0;
			GetRawInputData((HRAWINPUT)l_param, RID_INPUT, NULL, &dw_size, sizeof(RAWINPUTHEADER));

			if (dw_size > 0)
			{
				LPBYTE lpb = new BYTE[dw_size];
				if (GetRawInputData((HRAWINPUT)l_param, RID_INPUT, lpb, &dw_size, sizeof(RAWINPUTHEADER)) == dw_size)
				{
					RAWINPUT* raw = (RAWINPUT*)lpb;

					if (raw->header.dwType == RIM_TYPEMOUSE)
					{
						raw_mouse_input.dx += (float)raw->data.mouse.lLastX;
						raw_mouse_input.dy += (float)raw->data.mouse.lLastY;
						raw_mouse_input.updated = true;

						current_input.mouse.raw_dx = raw_mouse_input.dx;
						current_input.mouse.raw_dy = raw_mouse_input.dy;
					}
				}
				delete[] lpb;
			}
		}

		void handle_focus_gained()
		{
			current_input.window_focused = true;
			if (config.auto_capture_on_focus && previous_mouse_mode != MouseMode::VISIBLE)
			{
				apply_mouse_mode();
			}
		}

		void handle_focus_lost()
		{
			current_input.window_focused = false;
			if (cursor_captured)
			{
				release_mouse();
				if (current_mouse_mode == MouseMode::VISIBLE)
				{
					show_cursor();
				}
			}
			reset_input_state();
		}

		void reset_input_state()
		{
			for (int i = 0; i < KEY_COUNT; i++)
			{
				current_input.keyboard[i].down = false;
				current_input.keyboard[i].press_event = false;
				current_input.keyboard[i].release_event = false;
				current_input.keyboard[i].held_time = 0.0f;
			}

			current_input.mouse.left.down = false;
			current_input.mouse.right.down = false;
			current_input.mouse.middle.down = false;
			current_input.mouse.x1.down = false;
			current_input.mouse.x2.down = false;
			current_input.mouse.raw_dx = 0;
			current_input.mouse.raw_dy = 0;
			current_input.any_key_pressed = false;
		}

		void process_input_toggles()
		{
			if (is_key_pressed(config.key_toggle_mouse) && cursor_captured)
			{
				set_mouse_mode(MouseMode::VISIBLE);
			}

			if (is_key_pressed(config.key_toggle_cursor))
			{
				toggle_cursor_visibility();
			}

			if (is_key_down(KEY_ALT) && is_key_pressed(KEY_ENTER))
			{
				temporarily_show_cursor();
			}
		}

		void handle_idle_detection(float dt)
		{
			if (!config.show_cursor_after_idle || !cursor_captured)
			{
				idle_timer = 0.0f;
				return;
			}

			bool mouse_moved = (current_input.mouse.raw_dx != 0.0f || current_input.mouse.raw_dy != 0.0f);

			if (mouse_moved)
			{
				idle_timer = 0.0f;
			}
			else
			{
				idle_timer += dt;

				if (idle_timer >= config.idle_time_before_show)
				{
					temporarily_show_cursor();
					idle_timer = 0.0f;
				}
			}
		}

		void clear_mouse_events(InputMouseButton* button, float dt)
		{
			button->click_event = false;
			button->double_click_event = false;
			button->release_event = false;
			if (button->down)
			{
				button->held_time += dt;
			}
		}

		InputMouseButton* get_mouse_button(int button)
		{
			switch (button)
			{
			case 0: return &current_input.mouse.left;
			case 1: return &current_input.mouse.right;
			case 2: return &current_input.mouse.middle;
			case 3: return &current_input.mouse.x1;
			case 4: return &current_input.mouse.x2;
			default: return nullptr;
			}
		}

		float get_current_time()
		{
			LARGE_INTEGER current;
			QueryPerformanceCounter(&current);
			return (float)((current.QuadPart - perf_counter.QuadPart) / (double)perf_frequency.QuadPart);
		}
	};

	bool handle_windows_messages();

	static bool running = true;
	Win32Window* Win32Window::main_window = 0;

	static bool window_class_initialized;
	static const TCHAR* window_class_name = TEXT("APP WINDOW");

	static bool at_least_one_window_was_opened;
	static uint32 num_open_windows;

	static LRESULT CALLBACK window_call_back(
		_In_ HWND   hwnd,
		_In_ UINT   msg,
		_In_ WPARAM w_param,
		_In_ LPARAM l_param
	);

	static void set_fullscreen(HWND window_handle, bool fullscreen, WINDOWPLACEMENT& window_position)
	{
		DWORD style = GetWindowLong(window_handle, GWL_STYLE);

		if (fullscreen)
		{
			if (style & WS_OVERLAPPEDWINDOW)
			{
				MONITORINFO monitor_info = { sizeof(MONITORINFO) };
				if (GetWindowPlacement(window_handle, &window_position) &&
					GetMonitorInfo(MonitorFromWindow(window_handle, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
				{
					SetWindowLong(window_handle, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
					SetWindowPos(window_handle, HWND_TOP,
						monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
						monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
						monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
						SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				}
			}
		}
		else
		{
			if (!(style & WS_OVERLAPPEDWINDOW))
			{
				SetWindowLong(window_handle, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
				SetWindowPlacement(window_handle, &window_position);
				SetWindowPos(window_handle, 0, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
					SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			}
		}
	}

	static int dpi_scale(int value, UINT dpi)
	{
		return (int)((float)value * dpi / 96);
	}

	static int get_default_title_bar_height(HWND handle)
	{
		SIZE title_bar_size = { };
		const int top_and_bottom_borders = 2;
		HTHEME theme = OpenThemeData(handle, L"WINDOW");
		UINT dpi = GetDpiForWindow(handle);
		GetThemePartSize(theme, NULL, WP_CAPTION, CS_ACTIVE, NULL, TS_TRUE, &title_bar_size);
		CloseThemeData(theme);

		int height = dpi_scale(title_bar_size.cy, dpi) + top_and_bottom_borders;

		return height;
	}

	static RECT get_title_bar_rect(HWND handle, int32 title_bar_height)
	{
		RECT rect;
		GetClientRect(handle, &rect);
		rect.bottom = rect.top + title_bar_height;
		return rect;
	}

	static void center_rect_in_rect(RECT& to_center, const RECT& outer_rect)
	{
		int to_width = to_center.right - to_center.left;
		int to_height = to_center.bottom - to_center.top;
		int outer_width = outer_rect.right - outer_rect.left;
		int outer_height = outer_rect.bottom - outer_rect.top;

		int padding_x = (outer_width - to_width) / 2;
		int padding_y = (outer_height - to_height) / 2;

		to_center.left = outer_rect.left + padding_x;
		to_center.top = outer_rect.top + padding_y;
		to_center.right = to_center.left + to_width;
		to_center.bottom = to_center.top + to_height;
	}

	struct CustomTitlebarButtonRects
	{
		RECT close;
		RECT maximize;
		RECT minimize;
	};

	enum TitlebarButtonName
	{
		TITLEBAR_BUTTON_NONE = -1,
		TITLEBAR_BUTTON_CLOSE,
		TITLEBAR_BUTTON_MINIMIZE,
		TITLEBAR_BUTTON_MAXIMIZE,
	};

	static CustomTitlebarButtonRects get_title_bar_button_rects(HWND handle, int32 button_height)
	{
		UINT dpi = GetDpiForWindow(handle);
		CustomTitlebarButtonRects result;
		int button_width = dpi_scale(47, dpi);
		result.close = get_title_bar_rect(handle, button_height);

		result.close.left = result.close.right - button_width;
		result.maximize = result.close;
		result.maximize.left -= button_width;
		result.maximize.right -= button_width;
		result.minimize = result.maximize;
		result.minimize.left -= button_width;
		result.minimize.right -= button_width;
		return result;
	}

	int hit_test(POINT point, Win32Window* window)
	{
		if (!window->fullscreen)
		{
			RECT frame_rect;
			GetWindowRect(window->window_handle, &frame_rect);

			int32 border = 5;
			int32 corner = 10;

			if (point.x < frame_rect.left + border)
			{
				if (point.y < frame_rect.top + corner)
					return HTTOPLEFT;
				if (point.y > frame_rect.bottom - corner)
					return HTBOTTOMLEFT;
				return HTLEFT;
			}
			if (point.x > frame_rect.right - border)
			{
				if (point.y < frame_rect.top + corner)
					return HTTOPRIGHT;
				if (point.y > frame_rect.bottom - corner)
					return HTBOTTOMRIGHT;
				return HTRIGHT;
			}
			if (point.y < frame_rect.top + border)
			{
				if (point.x < frame_rect.left + corner)
					return HTTOPLEFT;
				if (point.x > frame_rect.right - corner)
					return HTTOPRIGHT;
				return HTTOP;
			}
			if (point.y > frame_rect.bottom - border)
			{
				if (point.x < frame_rect.left + corner)
					return HTBOTTOMLEFT;
				if (point.x > frame_rect.right - corner)
					return HTBOTTOMRIGHT;
				return HTBOTTOM;
			}

			CustomTitlebarButtonRects button_rects = get_title_bar_button_rects(window->window_handle, window->style.button_height);
			OffsetRect(&button_rects.close, frame_rect.left, frame_rect.top);
			OffsetRect(&button_rects.minimize, frame_rect.left, frame_rect.top);
			OffsetRect(&button_rects.maximize, frame_rect.left, frame_rect.top);

			if (PtInRect(&button_rects.close, point))
				return HTCLOSE;
			else if (PtInRect(&button_rects.maximize, point))
				return HTMAXBUTTON;
			else if (PtInRect(&button_rects.minimize, point))
				return HTMINBUTTON;

			if (point.y < frame_rect.top + window->style.title_bar_height)
			{
				int32 icon_padding = window->style.icon_padding;
				int32 icon_size = window->style.title_bar_height - icon_padding * 2;

				RECT icon_rect = { icon_padding, icon_padding, icon_padding + icon_size, icon_padding + icon_size };
				OffsetRect(&icon_rect, frame_rect.left, frame_rect.top);
				if (PtInRect(&icon_rect, point))
					return HTSYSMENU;

				return HTCAPTION;
			}
		}

		return HTCLIENT;
	}

	static HICON create_icon(const uint8* image, uint32 width, uint32 height)
	{
		BITMAPV5HEADER bi;

		ZeroMemory(&bi, sizeof(bi));
		bi.bV5Size = sizeof(bi);
		bi.bV5Width = width;
		bi.bV5Height = -(int32)height;
		bi.bV5Planes = 1;
		bi.bV5BitCount = 32;
		bi.bV5Compression = BI_BITFIELDS;
		bi.bV5RedMask = 0x00ff0000;
		bi.bV5GreenMask = 0x0000ff00;
		bi.bV5BlueMask = 0x000000ff;
		bi.bV5AlphaMask = 0xff000000;

		uint8* target = NULL;
		HDC dc = GetDC(NULL);
		HBITMAP color = CreateDIBSection(dc,
			(BITMAPINFO*)&bi,
			DIB_RGB_COLORS,
			(void**)&target,
			NULL,
			(DWORD)0);
		ReleaseDC(NULL, dc);

		if (!color)
		{
			std::cerr << "Win32: Failed to create RGBA bitmap.\n";
			return NULL;
		}

		HBITMAP mask = CreateBitmap(width, height, 1, 1, NULL);
		if (!mask)
		{
			std::cerr << "Failed to create mask bitmap.\n";
			DeleteObject(color);
			return NULL;
		}

		for (uint32 i = 0; i < width * height; ++i)
		{
			target[0] = image[2];
			target[1] = image[1];
			target[2] = image[0];
			target[3] = image[3];
			target += 4;
			image += 4;
		}

		ICONINFO ii = {};
		ii.fIcon = true;
		ii.hbmMask = mask;
		ii.hbmColor = color;

		HICON handle = CreateIconIndirect(&ii);

		DeleteObject(color);
		DeleteObject(mask);

		if (!handle)
		{
			std::cerr << "Failed to create icon.\n";
		}

		return handle;
	}

	bool Win32Window::initialize(const TCHAR* name, uint32 client_width, uint32 client_height, bool visible)
	{
		if (!window_class_initialized)
		{
			WNDCLASSEX wnd_class;
			ZeroMemory(&wnd_class, sizeof(WNDCLASSEX));
			wnd_class.cbSize = sizeof(WNDCLASSEX);
			wnd_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
			wnd_class.lpfnWndProc = window_call_back;
			wnd_class.hInstance = GetModuleHandle(NULL);
			wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
			wnd_class.lpszClassName = window_class_name;

			if (!RegisterClassEx(&wnd_class))
			{
				std::cerr << "Failed to create window class.\n";
				return false;
			}

			RegisterHotKey(0, 0, 0, VK_SNAPSHOT);
			RegisterHotKey(0, 1, MOD_CONTROL, VK_SNAPSHOT);

			window_class_initialized = true;
		}

		if (!window_handle)
		{
			++num_open_windows;

			this->client_width = client_width;
			this->client_height = client_height;

			DWORD window_style = WS_OVERLAPPEDWINDOW;

			RECT r = { 0, 0, (LONG)client_width, (LONG)client_height };
			AdjustWindowRect(&r, window_style, FALSE);
			int width = r.right - r.left;
			int height = r.bottom - r.top;

			window_handle = CreateWindowEx(0, window_class_name, name, window_style,
#if 1
				CW_USEDEFAULT, CW_USEDEFAULT,
#else
				0, 0
#endif
				width, height,
				0, 0, 0, 0);

			fullscreen = false;

			SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)this);

			if (!window_handle)
			{
				std::cerr << "Failed to create window.\n";
				return false;
			}

			at_least_one_window_was_opened = true;
		}

		if (!main_window)
		{
			main_window = this;
		}

		open = true;
		this->visible = visible;
		if (visible)
		{
			ShowWindow(window_handle, SW_SHOW);
		}

		return true;
	}

	bool Win32Window::initialize(HINSTANCE h_inst, const TCHAR* name, uint32 client_width, uint32 client_height, bool visible)
	{
		if (!window_class_initialized)
		{
			WNDCLASSEX wnd_class;
			ZeroMemory(&wnd_class, sizeof(WNDCLASSEX));
			wnd_class.cbSize = sizeof(WNDCLASSEX);
			wnd_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
			wnd_class.lpfnWndProc = window_call_back;
			wnd_class.hInstance = h_inst;
			wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
			wnd_class.lpszClassName = window_class_name;

			if (!RegisterClassEx(&wnd_class))
			{
				std::cerr << "Failed to create window class.\n";
				return false;
			}

			RegisterHotKey(0, 0, 0, VK_SNAPSHOT);
			RegisterHotKey(0, 1, MOD_CONTROL, VK_SNAPSHOT);

			window_class_initialized = true;
		}

		if (!window_handle)
		{
			++num_open_windows;

			this->client_width = client_width;
			this->client_height = client_height;

			DWORD window_style = WS_OVERLAPPEDWINDOW;

			RECT r = { 0, 0, (LONG)client_width, (LONG)client_height };
			AdjustWindowRect(&r, window_style, FALSE);
			int width = r.right - r.left;
			int height = r.bottom - r.top;

			window_handle = CreateWindowEx(0, window_class_name, name, window_style,
#if 1
				CW_USEDEFAULT, CW_USEDEFAULT,
#else
				0, 0
#endif
				width, height,
				0, 0, 0, 0);

			fullscreen = false;

			SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)this);

			if (!window_handle)
			{
				std::cerr << "Failed to create window.\n";
				return false;
			}

			at_least_one_window_was_opened = true;
		}

		if (!main_window)
		{
			main_window = this;
		}

		open = true;
		this->visible = visible;
		/*if (visible)
		{
			ShowWindow(windowHandle, SW_SHOW);
		}*/

		return true;
	}

	void Win32Window::shutdown()
	{
		if (window_handle)
		{
			HWND handle = window_handle;
			window_handle = 0;
			DestroyWindow(handle);
		}

		fullscreen = false;
		open = false;
		visible = false;
	}

	void Win32Window::toggle_fullscreen()
	{
		fullscreen = !fullscreen;
		set_fullscreen(window_handle, fullscreen, window_position);
	}

	void Win32Window::set_file_drop_callback(std::function<void(const fs::path&)> cb)
	{
		if (!file_drop_callback)
		{
			DragAcceptFiles(window_handle, true);
		}
		file_drop_callback = cb;
	}

	Win32Window::Win32Window(Win32Window&& o) noexcept
	{
		open = o.open;
		window_handle = o.window_handle;
		client_width = o.client_width;
		client_height = o.client_height;
		window_position = o.window_position;
		fullscreen = o.fullscreen;

		o.window_handle = 0;

		if (window_handle && open)
		{
			SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)this);
		}

		if (main_window == &o)
		{
			main_window = this;
		}
	}

	Win32Window::~Win32Window()
	{
		shutdown();
		if (input_handler != nullptr)
		{
			delete input_handler;
		}
	}

	void Win32Window::make_active()
	{
		SetForegroundWindow(window_handle);
	}

	static void redraw_window_frame(HWND window_handle)
	{
		RECT rect;
		GetClientRect(window_handle, &rect);

		AdjustWindowRectExForDpi(&rect, GetWindowLong(window_handle, GWL_STYLE), FALSE,
			0,
			GetDpiForWindow(window_handle));

		ClientToScreen(window_handle, (POINT*)&rect.left);
		ClientToScreen(window_handle, (POINT*)&rect.right);
		SetWindowPos(window_handle, HWND_TOP,
			rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER);

		handle_windows_messages(); // Handle messages, so that frame is custom right from the start
	}

	void Win32Window::set_custom_window_style(CustomWindowStyle style)
	{
		custom_window_style = true;

		if (style.title_bar_height == -1)
		{
			style.title_bar_height = get_default_title_bar_height(window_handle);
		}
		if (style.button_height == -1)
		{
			style.button_height = style.title_bar_height;
		}
		if (style.border_width_left_and_right == -1)
		{
			UINT dpi = GetDpiForWindow(window_handle);
			style.border_width_left_and_right = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
		}
		if (style.border_width_bottom == -1)
		{
			UINT dpi = GetDpiForWindow(window_handle);
			style.border_width_bottom = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
		}
		this->style = style;

		redraw_window_frame(window_handle);
	}

	void Win32Window::reset_to_default_window_style()
	{
		custom_window_style = false;
		redraw_window_frame(window_handle);
	}

	const UserInput& Win32Window::get_current_frame_input()
	{
		if (input_handler == nullptr)
		{
			static UserInput dummy;
			return dummy;
		}

		return input_handler->get_input();
	}

	void Win32Window::set_icon(const fs::path& filepath)
	{
		DirectX::ScratchImage scratch_image;
		D3D12_RESOURCE_DESC desc;

		if (load_image_from_file(filepath, IMAGE_LOAD_FLAGS_CACHE_TO_DDS, scratch_image, desc) && scratch_image.GetImageCount() > 0)
		{
			const auto& image = scratch_image.GetImages()[0];
			ASSERT(get_number_of_channels(image.format) == 4);
			uint8* pixels = image.pixels;
			uint32 width = (uint32)image.width;
			uint32 height = (uint32)image.height;

			HICON icon = create_icon(pixels, width, height);

			if (custom_icon)
				DestroyIcon(custom_icon);
			custom_icon = icon;

			SendMessage(window_handle, WM_SETICON, ICON_BIG, (LPARAM)icon);
			SendMessage(window_handle, WM_SETICON, ICON_SMALL, (LPARAM)icon);
		}
	}

	void Win32Window::change_title(const TCHAR* format, ...)
	{
		TCHAR title_buffer[128];

		va_list arg;
		va_start(arg, format);
		_vstprintf_s(title_buffer, format, arg);
		va_end(arg);

		SetWindowText(window_handle, title_buffer);
	}

	void Win32Window::init_input()
	{
		if (input_handler != nullptr)
		{
			return;
		}

		input_handler = new Win32InputHandler();
		input_handler->initialize(window_handle);

		InputConfig config;
		config.mouse_sensitivity = 0.15f;
		config.invert_y = false;
		config.right_click_to_capture = true;
		config.auto_capture_on_focus = true;
		config.show_cursor_after_idle = true;
		config.idle_time_before_show = 3.0f;
		input_handler->set_config(config);
	}

	void Win32Window::begin_frame(float dt)
	{
		if (input_handler == nullptr)
		{
			return;
		}

		input_handler->begin_frame(dt);
	}

	void Win32Window::update_input(float dt)
	{
		if (input_handler == nullptr)
		{
			return;
		}

		input_handler->update(dt);

		const UserInput& input = input_handler->get_input();

		if (input.keyboard[KEY_TAB].press_event)
		{
			input_handler->capture_mouse();
		}

		if (input.mouse.right.click_event)
		{
			input_handler->toggle_mouse_capture();
		}
	}

	void Win32Window::toggle_visibility()
	{
		visible = !visible;
		if (visible)
		{
			ShowWindow(window_handle, SW_SHOW);
		}
		else
		{
			ShowWindow(window_handle, SW_HIDE);
		}
	}

	void Win32Window::maximize()
	{
		ShowWindow(window_handle, SW_MAXIMIZE);
	}

	void Win32Window::set_minimum_size(int32 minimum_width, int32 minimum_height)
	{
		this->minimum_width = minimum_width;
		this->minimum_height = minimum_height;
	}

	void Win32Window::move_to(int x, int y)
	{
		SetWindowPos(window_handle, HWND_TOP, x, y, client_width, client_height, SWP_NOSIZE);
	}

	void Win32Window::move_to_screen_id(int screen_id)
	{
		DISPLAY_DEVICEA disp_device = { 0 };
		disp_device.cb = sizeof(disp_device);

		DEVMODEA dev_mode = { 0 };
		dev_mode.dmSize = sizeof(dev_mode);

		if (EnumDisplayDevicesA(NULL, screen_id, &disp_device, 0))
		{
			if (EnumDisplaySettingsExA(disp_device.DeviceName, ENUM_CURRENT_SETTINGS, &dev_mode, NULL))
			{
				move_to(dev_mode.dmPosition.x, dev_mode.dmPosition.y);
			}
		}
	}

	struct MonitorIterator
	{
		DISPLAY_DEVICEA disp_device;
		DEVMODEA dev_mode;
		DWORD screen_id;

		MonitorIterator();

		bool step(MonitorInfo& info);
	};

	void Win32Window::move_to_monitor(const std::string& unique_id)
	{
		MonitorIterator it;
		MonitorInfo monitor;
		while (it.step(monitor))
		{
			if (monitor.unique_id == unique_id)
			{
				move_to(monitor.x, monitor.y);
				return;
			}
		}
	}

	void Win32Window::move_to_monitor(const MonitorInfo& monitor)
	{
		move_to(monitor.x, monitor.y);
	}

	MonitorIterator::MonitorIterator()
	{
		ZeroMemory(&disp_device, sizeof(disp_device));
		ZeroMemory(&dev_mode, sizeof(dev_mode));
		disp_device.cb = sizeof(disp_device);
		dev_mode.dmSize = sizeof(dev_mode);
		screen_id = 0;
	}

	static std::string convert_unique_id_to_folder_friendly_name(const std::string& unique_id)
	{
		std::string result = unique_id;
		std::replace(result.begin(), result.end(), '\\', '_');
		std::replace(result.begin(), result.end(), '?', '_');
		return result;
	}

	bool MonitorIterator::step(MonitorInfo& info)
	{
		bool result = false;

		if (EnumDisplayDevicesA(NULL, screen_id, &disp_device, 0))
		{
			char name[sizeof(disp_device.DeviceName)];
			strcpy_s(name, disp_device.DeviceName);
			if (EnumDisplayDevicesA(name, 0, &disp_device, EDD_GET_DEVICE_INTERFACE_NAME))
			{
				if (EnumDisplaySettingsExA(name, ENUM_CURRENT_SETTINGS, &dev_mode, NULL))
				{
					info.x = dev_mode.dmPosition.x;
					info.y = dev_mode.dmPosition.y;
					info.width = dev_mode.dmPelsWidth;
					info.height = dev_mode.dmPelsHeight;
					info.screen_id = screen_id;
					info.unique_id = convert_unique_id_to_folder_friendly_name(disp_device.DeviceID);
					info.name = disp_device.DeviceString;
					result = true;
				}
			}
		}

		++screen_id;

		return result;
	}

	NODISCARD std::vector<MonitorInfo> get_all_display_devices()
	{
		std::vector<MonitorInfo> result;

		MonitorIterator it;
		MonitorInfo monitor;
		while (it.step(monitor))
		{
			result.push_back(monitor);
		}

		return result;
	}

	NODISCARD std::vector<MonitorInfo> get_all_display_devices(uint32 width, uint32 height)
	{
		std::vector<MonitorInfo> result;

		for (MonitorInfo& monitor : get_all_display_devices())
		{
			if (width == monitor.width && height == monitor.height)
			{
				result.push_back(monitor);
			}
		}

		return result;
	}

	void set_main_window(Win32Window* window)
	{
		Win32Window::main_window = window;
	}

	LRESULT CALLBACK window_call_back(
		_In_ HWND   hwnd,
		_In_ UINT   msg,
		_In_ WPARAM w_param,
		_In_ LPARAM l_param
	)
	{
		LRESULT result = 0;

		Win32Window* window = (Win32Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		if ((msg == WM_MOUSELEAVE || msg == WM_NCMOUSELEAVE || msg == WM_MOUSEMOVE)
			&& window && window->custom_window_style)
		{
			if (window->hovered_button != TITLEBAR_BUTTON_NONE)
			{
				window->hovered_button = TITLEBAR_BUTTON_NONE;
				//InvalidateRect(0, 0, FALSE);
				RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_FRAME);
			}

			window->tracking_mouse = false;
		}

		if (window != nullptr && window->input_handler != nullptr)
		{
			window->input_handler->process_message(msg, w_param, l_param);
		}

		if (handle_imgui_input(hwnd, msg, w_param, l_param))
			return true;

		switch (msg)
		{
			// The default window procedure will play a system notification sound 
			// when pressing the Alt+Enter keyboard combination if this message is 
			// not handled.
		case WM_SYSCHAR:
			break;
		case WM_SIZE:
		{
			if (window && window->open)
			{
				window->client_width = LOWORD(l_param);
				window->client_height = HIWORD(l_param);
				window->on_resize();

				if (window->custom_window_style)
				{
					if (IsZoomed(hwnd))
					{
						SetWindowRgn(hwnd, 0, true);
					}
					else
					{
						RECT frame_rect;
						GetWindowRect(hwnd, &frame_rect);

						HRGN region = CreateRectRgn(0, 0, frame_rect.right - frame_rect.left, frame_rect.bottom - frame_rect.top);
						SetWindowRgn(hwnd, region, true);
						DeleteObject(region);
					}
				}
			}
		} break;
		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
		} break;
		case WM_DESTROY:
		{
			if (window && window->window_handle && window->open)
			{
				window->open = false;
				--num_open_windows;
				window->shutdown();
			}
		} break;

		case WM_ACTIVATEAPP:
		{
			if (w_param)
			{
				//std::cout << "Activated\n";
			}
			else
			{
				//std::cout << "Deactivated\n";
			}
			if (window && window->custom_window_style)
			{
				//InvalidateRect(0, 0, FALSE);
				RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_FRAME);
			}
		} break;

		case WM_PAINT:
		{
			if (window)
			{
				// For software windows, we draw the content in the client area.
				SoftwareWindow* s_window = dynamic_cast<SoftwareWindow*>(window);

				if (s_window)
				{
					const uint8* image = s_window->buffer;
					if (image)
					{
						PAINTSTRUCT ps;
						HDC hdc = BeginPaint(hwnd, &ps);
						StretchDIBits(hdc,
#if 0
							0, 0, window->client_width, window->client_height,
							0, 0, s_window->bitmap_info->bmiHeader.biWidth, abs(s_window->bitmap_info->bmiHeader.biHeight),
#else
							0, window->client_height - 1, window->client_width, -(int)window->client_height,
							s_window->blit_x, s_window->blit_y, s_window->blit_width, s_window->blit_height,
#endif
							image, s_window->bitmap_info, DIB_RGB_COLORS, SRCCOPY);
						EndPaint(hwnd, &ps);
					}
					else
					{
						result = DefWindowProc(hwnd, msg, w_param, l_param);
					}
				}
				else
				{
					result = DefWindowProc(hwnd, msg, w_param, l_param);
				}
			}
		} break;

		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* info = (MINMAXINFO*)l_param;

			if (window)
			{
				if (window->minimum_width != -1)
					info->ptMinTrackSize.x = window->minimum_width;
				if (window->minimum_height != -1)
					info->ptMinTrackSize.y = window->minimum_height;
			}
			return 0;
		} break;

		case WM_NCACTIVATE:
		case WM_NCPAINT:
		{
			if (!window || !window->custom_window_style)
				return DefWindowProc(hwnd, msg, w_param, l_param);

			HDC hdc = GetDCEx(hwnd, 0, DCX_WINDOW);

			// Render bars and buttons
			RECT frame_rect;
			GetWindowRect(hwnd, &frame_rect);

			RECT client_rect;
			GetClientRect(hwnd, &client_rect);

			ClientToScreen(hwnd, (POINT*)&client_rect.left);
			ClientToScreen(hwnd, (POINT*)&client_rect.right);

			OffsetRect(&client_rect, -frame_rect.left, -frame_rect.top);
			OffsetRect(&frame_rect, -frame_rect.left, -frame_rect.top);

			bool has_focus = GetActiveWindow() == hwnd;
			bool is_maximized = IsZoomed(hwnd);

			COLORREF title_bar_color = has_focus
				? RGB(window->style.title_bar_rgb[0], window->style.title_bar_rgb[1], window->style.title_bar_rgb[2])
				: RGB(window->style.title_bar_unfocused_rgb[0], window->style.title_bar_unfocused_rgb[1], window->style.title_bar_unfocused_rgb[2]);
			HBRUSH title_bar_brush = CreateSolidBrush(title_bar_color);
			HPEN title_bar_pen = CreatePen(PS_SOLID, 0, title_bar_color);

			SelectObject(hdc, title_bar_brush);
			SelectObject(hdc, title_bar_pen);

			Rectangle(hdc, frame_rect.left, frame_rect.top, frame_rect.right, client_rect.top);			// Top bar.
			Rectangle(hdc, frame_rect.left, client_rect.top, client_rect.left, client_rect.bottom);		// Left bar.
			Rectangle(hdc, client_rect.right, client_rect.top, frame_rect.right, client_rect.bottom);	// Right bar.
			Rectangle(hdc, frame_rect.left, client_rect.bottom, frame_rect.right, frame_rect.bottom);	// Bottom bar.

			COLORREF item_color = RGB(window->style.title_text_rgb[0], window->style.title_text_rgb[1], window->style.title_text_rgb[2]);

			HBRUSH button_icon_brush = CreateSolidBrush(item_color);
			HPEN button_icon_pen = CreatePen(PS_SOLID, 1, item_color);

			HBRUSH hover_brush = CreateSolidBrush(RGB(window->style.button_hover_rgb[0], window->style.button_hover_rgb[1], window->style.button_hover_rgb[2]));

			CustomTitlebarButtonRects button_rects = get_title_bar_button_rects(hwnd, window->style.button_height);

			UINT dpi = GetDpiForWindow(hwnd);
			int icon_dimension = dpi_scale(10, dpi);

			// Minimize button
			{
				if (window->hovered_button == TITLEBAR_BUTTON_MINIMIZE)
				{
					FillRect(hdc, &button_rects.minimize, hover_brush);
				}
				RECT icon_rect = { 0 };
				icon_rect.right = icon_dimension;
				icon_rect.bottom = 1;
				center_rect_in_rect(icon_rect, button_rects.minimize);
				FillRect(hdc, &icon_rect, button_icon_brush);
			}

			// Maximize button.
			{
				HBRUSH front_brush = title_bar_brush;
				if (window->hovered_button == TITLEBAR_BUTTON_MAXIMIZE)
				{
					FillRect(hdc, &button_rects.maximize, hover_brush);
					front_brush = hover_brush;
				}
				RECT icon_rect = { 0 };
				icon_rect.right = icon_dimension;
				icon_rect.bottom = icon_dimension;
				center_rect_in_rect(icon_rect, button_rects.maximize);
				SelectObject(hdc, button_icon_pen);
				SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
				Rectangle(hdc, icon_rect.left, icon_rect.top, icon_rect.right, icon_rect.bottom);

				if (is_maximized)
				{
					SelectObject(hdc, front_brush);
					Rectangle(hdc, icon_rect.left - 2, icon_rect.top + 2, icon_rect.right - 2, icon_rect.bottom + 2);
				}
			}

			// Close button
			{
				HPEN custom_pen = 0;
				if (window->hovered_button == TITLEBAR_BUTTON_CLOSE)
				{
					// Highlight button on hover
					HBRUSH fill_brush = CreateSolidBrush(RGB(window->style.close_button_hover_rgb[0], window->style.close_button_hover_rgb[1], window->style.close_button_hover_rgb[2]));
					FillRect(hdc, &button_rects.close, fill_brush);
					DeleteObject(fill_brush);
					custom_pen = CreatePen(PS_SOLID, 1, RGB(window->style.close_button_hover_stroke_rgb[0], window->style.close_button_hover_stroke_rgb[1], window->style.close_button_hover_stroke_rgb[2]));
					SelectObject(hdc, custom_pen);
				}
				RECT icon_rect = { 0, 0, icon_dimension, icon_dimension };
				center_rect_in_rect(icon_rect, button_rects.close);
				MoveToEx(hdc, icon_rect.left, icon_rect.top, NULL);
				LineTo(hdc, icon_rect.right + 1, icon_rect.bottom + 1);
				MoveToEx(hdc, icon_rect.left, icon_rect.bottom, NULL);
				LineTo(hdc, icon_rect.right + 1, icon_rect.top - 1);
				if (custom_pen)
				{
					DeleteObject(custom_pen);
				}
			}

			DeleteObject(title_bar_brush);
			DeleteObject(title_bar_pen);

			DeleteObject(hover_brush);
			DeleteObject(button_icon_pen);
			DeleteObject(button_icon_brush);


			// Extra padding for title and icon if window is maximized
			int32 left_and_top_padding = 0;
			if (is_maximized)
			{
				left_and_top_padding = 5;
			}

			HTHEME theme = OpenThemeData(hwnd, L"WINDOW");

			// Draw window title
			LOGFONT logical_font;
			HFONT old_font = NULL;
			if (SUCCEEDED(GetThemeSysFont(theme, TMT_CAPTIONFONT, &logical_font)))
			{
				HFONT theme_font = CreateFontIndirect(&logical_font);
				old_font = (HFONT)SelectObject(hdc, theme_font);
			}

			wchar_t title_text_buffer[255] = { 0 };
			GetWindowTextW(hwnd, title_text_buffer, arraysize(title_text_buffer));
			RECT title_bar_text_rect = { window->style.title_left_offset + left_and_top_padding, left_and_top_padding, button_rects.minimize.left - 10 + left_and_top_padding, window->style.title_bar_height };
			DTTOPTS draw_theme_options = { sizeof(draw_theme_options) };
			draw_theme_options.dwFlags = DTT_TEXTCOLOR;
			draw_theme_options.crText = item_color;
			DrawThemeTextEx(
				theme,
				hdc,
				0, 0,
				title_text_buffer,
				-1,
				DT_VCENTER | DT_SINGLELINE | DT_WORD_ELLIPSIS,
				&title_bar_text_rect,
				&draw_theme_options
			);

			if (old_font)
			{
				SelectObject(hdc, old_font);
			}
			CloseThemeData(theme);

			// Draw window icon
			HICON icon = window->custom_icon;
			if (!icon)
			{
				icon = LoadIcon(NULL, IDI_APPLICATION);
			}

			if (icon)
			{
				int32 icon_padding = window->style.icon_padding;
				int32 icon_size = window->style.title_bar_height - icon_padding * 2;

				DrawIconEx(hdc, icon_padding + left_and_top_padding, icon_padding + left_and_top_padding, icon, icon_size, icon_size, 0, NULL, DI_NORMAL);
			}

			ReleaseDC(hwnd, hdc);

			return true;
		} break;

		case WM_NCHITTEST:
		{
			if (!window || !window->custom_window_style)
			{
				return DefWindowProc(hwnd, msg, w_param, l_param);
			}

			POINT point = { LOWORD(l_param), HIWORD(l_param) };
			return hit_test(point, window);
		} break;

		case WM_NCCALCSIZE:
		{
			if (!window || !window->custom_window_style)
			{
				return DefWindowProc(hwnd, msg, w_param, l_param);
			}

			RECT* rect = (RECT*)l_param;
			if (rect->top > -10000) // Minimized windows seem to have a top of -32000.
			{
				rect->left += window->style.border_width_left_and_right;
				rect->right -= window->style.border_width_left_and_right;
				rect->bottom -= window->style.border_width_bottom;

				if (!window->fullscreen)
				{
					rect->top += window->style.title_bar_height;
				}
				else
				{
					rect->top += window->style.border_width_bottom;
				}
			}
		} break;

		case WM_NCMOUSEMOVE:
		{
			if (window && window->custom_window_style)
			{
				POINT cursor_point = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };

				int hovered_button = TITLEBAR_BUTTON_NONE;
				int hit = hit_test(cursor_point, window);

				if (hit == HTCLOSE)
				{
					hovered_button = TITLEBAR_BUTTON_CLOSE;
				}
				else if (hit == HTMAXBUTTON)
				{
					hovered_button = TITLEBAR_BUTTON_MAXIMIZE;
				}
				else if (hit == HTMINBUTTON)
				{
					hovered_button = TITLEBAR_BUTTON_MINIMIZE;
				}

				if (hovered_button != window->hovered_button)
				{
					window->hovered_button = hovered_button;
					//InvalidateRect(0, 0, FALSE);
					RedrawWindow(hwnd, 0, 0, RDW_INVALIDATE | RDW_FRAME);
				}

				if (!window->tracking_mouse)
				{
					TRACKMOUSEEVENT event_track = { sizeof(TRACKMOUSEEVENT) };
					event_track.dwFlags = TME_NONCLIENT;
					event_track.hwndTrack = hwnd;
					event_track.dwHoverTime = HOVER_DEFAULT;

					TrackMouseEvent(&event_track);

					window->tracking_mouse = true;
				}
				return 0;
			}
			return DefWindowProc(hwnd, msg, w_param, l_param);
		}

		case WM_NCLBUTTONDOWN:
		{
			// Clicks on buttons will be handled in WM_NCLBUTTONUP, but we still need
			// to remove default handling of the click to avoid it counting as drag
			if (window && window->custom_window_style && window->hovered_button != TITLEBAR_BUTTON_NONE)
				return 0;
			// Default handling allows for dragging and double click to maximize
			return DefWindowProc(hwnd, msg, w_param, l_param);
		}

		case WM_NCLBUTTONUP:
		{
			if (window && window->custom_window_style)
			{
				if (window->hovered_button == TITLEBAR_BUTTON_CLOSE)
				{
					window->hovered_button = TITLEBAR_BUTTON_NONE;
					PostMessage(hwnd, WM_CLOSE, 0, 0);
					return 0;
				}
				else if (window->hovered_button == TITLEBAR_BUTTON_MINIMIZE)
				{
					window->hovered_button = TITLEBAR_BUTTON_NONE;
					ShowWindow(hwnd, SW_MINIMIZE);
					return 0;
				}
				else if (window->hovered_button == TITLEBAR_BUTTON_MAXIMIZE)
				{
					window->hovered_button = TITLEBAR_BUTTON_NONE;
					int mode = IsZoomed(hwnd) ? SW_NORMAL : SW_MAXIMIZE;
					ShowWindow(hwnd, mode);
					return 0;
				}
			}
			return DefWindowProc(hwnd, msg, w_param, l_param);
		}

		case WM_CREATE:
		{
			RECT rect;
			GetWindowRect(hwnd, &rect);

			// Inform the application of the frame change to force redrawing with the new
			// client area that is extended into the title bar
			SetWindowPos(
				hwnd, NULL,
				rect.left, rect.top,
				rect.right - rect.left, rect.bottom - rect.top,
				SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE
			);
		} break;

		case WM_DROPFILES:
		{
			if (window && window->file_drop_callback)
			{
				HDROP hdrop = (HDROP)w_param;

				wchar next_file[MAX_PATH];
				uint32 num_files = DragQueryFileW(hdrop, -1, NULL, 0);

				for (uint32 i = 0; i < num_files; ++i)
				{
					if (DragQueryFileW(hdrop, i, next_file, MAX_PATH) > 0)
					{
						window->file_drop_callback(next_file);
					}
				}

				DragFinish(hdrop);
			}
		} break;

		default:
		{
			result = DefWindowProc(hwnd, msg, w_param, l_param);
		} break;
		}

		return result;
	}

	bool handle_windows_messages()
	{
		MSG msg = { 0 };
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_HOTKEY)
			{
				ImGui::GetIO().KeysDown[VK_SNAPSHOT] = true;
			}
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (at_least_one_window_was_opened && num_open_windows == 0)
		{
			running = false;
		}

		if (Win32Window::main_window && !Win32Window::main_window->open)
		{
			running = false;
		}

		return running;
	}
}