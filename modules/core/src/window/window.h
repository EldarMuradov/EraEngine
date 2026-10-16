// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include <functional>

namespace era_engine
{
	struct ERA_CORE_API CustomWindowStyle
	{
		// Default values match the current ImGui style.

		uint8 title_bar_rgb[3] = { 25, 27, 28 };
		uint8 title_bar_unfocused_rgb[3] = { 45, 47, 48 };
		uint8 title_text_rgb[3] = { 255, 255, 255 };
		uint8 button_hover_rgb[3] = { 80, 80, 80 };

		uint8 close_button_hover_rgb[3] = { 0xCC, 0, 0 };
		uint8 close_button_hover_stroke_rgb[3] = { 255, 255, 255 };

		int32 title_left_offset = 30;
		int32 title_bar_height = -1; // Set to -1 for default title bar height.
		int32 button_height = -1; // Set to -1 for buttons as tall as the title bar.
		int32 icon_padding = 5; // Padding around icon. Size of icon will be titleBarHeight - iconPadding * 2.
		int32 border_width_left_and_right = 0; // Set to -1 for default border width.
		int32 border_width_bottom = 0; // Set to -1 for default border width. Will be used for top when window is full-screen.

		bool prevent_rounded_top_corners = true;
	};

	class Win32InputHandler;
	struct UserInput;

	struct ERA_CORE_API Win32Window
	{
		Win32Window() = default;
		Win32Window(Win32Window&) = delete;
		Win32Window(Win32Window&& o) noexcept;

		virtual ~Win32Window();

		bool initialize(const TCHAR* name, uint32 client_width, uint32 client_height, bool visible = true);
		bool initialize(HINSTANCE h_inst, const TCHAR* name, uint32 client_width, uint32 client_height, bool visible = true);
		virtual void shutdown();

		virtual void swap_buffers() = 0;
		virtual void toggle_fullscreen();
		void set_file_drop_callback(std::function<void(const fs::path&)> cb);

		void toggle_visibility();

		void maximize();

		void set_minimum_size(int32 minimum_width = -1, int32 minimum_height = -1); // -1 means default Windows limits.

		void move_to(int x, int y);
		void move_to_screen_id(int screen_id);
		void move_to_monitor(const std::string& unique_id);
		void move_to_monitor(const struct MonitorInfo& monitor);

		void make_active();

		void set_custom_window_style(CustomWindowStyle style = {});
		void reset_to_default_window_style();
		void set_icon(const fs::path& filepath);
		void change_title(const TCHAR* format, ...);

		void init_input();

		void begin_frame(float dt);
		void update_input(float dt);

		const UserInput& get_current_frame_input();

		uint32 client_width, client_height;
		HWND window_handle = 0;

		Win32InputHandler* input_handler = nullptr;

	protected:
		// Internal callbacks.
		virtual void on_resize() {}
		virtual void on_move() {}
		virtual void on_window_display_change() {}

		WINDOWPLACEMENT window_position;

		bool fullscreen = false;
		bool open = false;
		bool visible = false;

		bool custom_window_style = false;
		CustomWindowStyle style;
		HICON custom_icon = 0;

		int32 minimum_width = -1;
		int32 minimum_height = -1;

		std::function<void(const fs::path&)> file_drop_callback;

		int hovered_button = -1; // Only used for custom window styles.
		bool tracking_mouse = false;

		static Win32Window* main_window;

		friend int hit_test(POINT point, Win32Window* window);
		friend LRESULT CALLBACK window_call_back(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM w_param, _In_ LPARAM l_param);
		friend void set_main_window(Win32Window* window);
		friend bool handle_windows_messages();
	};

	struct ERA_CORE_API MonitorInfo
	{
		// This is specific for the actual physical monitor / projector.
		std::string name;
		std::string unique_id;

		// This can change from run to run!
		int screen_id;
		int x, y;
		uint32 width, height;
	};

	NODISCARD std::vector<MonitorInfo> get_all_display_devices();
	NODISCARD std::vector<MonitorInfo> get_all_display_devices(uint32 width, uint32 height);

	void set_main_window(Win32Window* window);
}