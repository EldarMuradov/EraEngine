#pragma once

#include "core_api.h"

namespace era_engine
{
    enum class MouseMode
    {
        VISIBLE,        // Normal cursor, free movement
        HIDDEN,         // Cursor hidden but not captured
        CAPTURED,       // Cursor hidden and locked to center
        CAPTURED_RAW     // Captured with raw input
    };

    enum KeyCode : uint32
    {
        KEY_NONE = 0x00,
        KEY_LEFT_MOUSE = VK_LBUTTON,
        KEY_RIGHT_MOUSE = VK_RBUTTON,
        KEY_MIDDLE_MOUSE = VK_MBUTTON,
        KEY_X1_MOUSE = VK_XBUTTON1,
        KEY_X2_MOUSE = VK_XBUTTON2,
        KEY_BACKSPACE = VK_BACK,
        KEY_TAB = VK_TAB,
        KEY_ENTER = VK_RETURN,
        KEY_SHIFT = VK_SHIFT,
        KEY_CTRL = VK_CONTROL,
        KEY_ALT = VK_MENU,
        KEY_PAUSE = VK_PAUSE,
        KEY_CAPS = VK_CAPITAL,
        KEY_ESC = VK_ESCAPE,
        KEY_SPACE = VK_SPACE,
        KEY_PAGE_UP = VK_PRIOR,
        KEY_PAGE_DOWN = VK_NEXT,
        KEY_END = VK_END,
        KEY_HOME = VK_HOME,
        KEY_LEFT = VK_LEFT,
        KEY_UP = VK_UP,
        KEY_RIGHT = VK_RIGHT,
        KEY_DOWN = VK_DOWN,
        KEY_PRINT = VK_SNAPSHOT,
        KEY_INSERT = VK_INSERT,
        KEY_DELETE = VK_DELETE,
        KEY_0 = '0', KEY_1 = '1', KEY_2 = '2', KEY_3 = '3', KEY_4 = '4',
        KEY_5 = '5', KEY_6 = '6', KEY_7 = '7', KEY_8 = '8', KEY_9 = '9',
        KEY_A = 'A', KEY_B = 'B', KEY_C = 'C', KEY_D = 'D', KEY_E = 'E',
        KEY_F = 'F', KEY_G = 'G', KEY_H = 'H', KEY_I = 'I', KEY_J = 'J',
        KEY_K = 'K', KEY_L = 'L', KEY_M = 'M', KEY_N = 'N', KEY_O = 'O',
        KEY_P = 'P', KEY_Q = 'Q', KEY_R = 'R', KEY_S = 'S', KEY_T = 'T',
        KEY_U = 'U', KEY_V = 'V', KEY_W = 'W', KEY_X = 'X', KEY_Y = 'Y',
        KEY_Z = 'Z',
        KEY_NUMPAD_0 = VK_NUMPAD0, KEY_NUMPAD_1 = VK_NUMPAD1,
        KEY_NUMPAD_2 = VK_NUMPAD2, KEY_NUMPAD_3 = VK_NUMPAD3,
        KEY_NUMPAD_4 = VK_NUMPAD4, KEY_NUMPAD_5 = VK_NUMPAD5,
        KEY_NUMPAD_6 = VK_NUMPAD6, KEY_NUMPAD_7 = VK_NUMPAD7,
        KEY_NUMPAD_8 = VK_NUMPAD8, KEY_NUMPAD_9 = VK_NUMPAD9,
        KEY_MULTIPLY = VK_MULTIPLY, KEY_ADD = VK_ADD,
        KEY_SUBTRACT = VK_SUBTRACT, KEY_DECIMAL = VK_DECIMAL,
        KEY_DIVIDE = VK_DIVIDE,
        KEY_F1 = VK_F1, KEY_F2 = VK_F2, KEY_F3 = VK_F3, KEY_F4 = VK_F4,
        KEY_F5 = VK_F5, KEY_F6 = VK_F6, KEY_F7 = VK_F7, KEY_F8 = VK_F8,
        KEY_F9 = VK_F9, KEY_F10 = VK_F10, KEY_F11 = VK_F11, KEY_F12 = VK_F12,
        KEY_NUM_LOCK = VK_NUMLOCK, KEY_SCROLL_LOCK = VK_SCROLL,
        KEY_LEFT_SHIFT = VK_LSHIFT, KEY_RIGHT_SHIFT = VK_RSHIFT,
        KEY_LEFT_CTRL = VK_LCONTROL, KEY_RIGHT_CTRL = VK_RCONTROL,
        KEY_LEFT_ALT = VK_LMENU, KEY_RIGHT_ALT = VK_RMENU,
        KEY_SEMICOLON = VK_OEM_1, KEY_PLUS = VK_OEM_PLUS,
        KEY_COMMA = VK_OEM_COMMA, KEY_MINUS = VK_OEM_MINUS,
        KEY_PERIOD = VK_OEM_PERIOD, KEY_SLASH = VK_OEM_2,
        KEY_TILDE = VK_OEM_3, KEY_LEFT_BRACKET = VK_OEM_4,
        KEY_BACKSLASH = VK_OEM_5, KEY_RIGHT_BRACKET = VK_OEM_6,
        KEY_QUOTE = VK_OEM_7,
        KEY_COUNT = 256
    };

    struct ERA_CORE_API InputKey
    {
        bool down = false;
        bool press_event = false;
        bool release_event = false;
        float held_time = 0.0f;
    };

    struct ERA_CORE_API InputMouseButton
    {
        bool down = false;
        bool click_event = false;
        bool double_click_event = false;
        bool release_event = false;
        float held_time = 0.0f;
        uint32 click_count = 0;
    };

    struct ERA_CORE_API MouseInput
    {
        InputMouseButton left;
        InputMouseButton right;
        InputMouseButton middle;
        InputMouseButton x1;
        InputMouseButton x2;
        float scroll = 0.0f;
        float scroll_delta = 0.0f;

        int32 x = 0;
        int32 y = 0;
        int32 dx = 0;
        int32 dy = 0;

        // Raw input deltas
        float raw_dx = 0.0f;
        float raw_dy = 0.0f;

        float rel_x = 0.0f;
        float rel_y = 0.0f;
        float reldx = 0.0f;
        float reldy = 0.0f;

        bool visible = true;
        bool captured = false;
    };

    struct ERA_CORE_API UserInput
    {
        InputKey keyboard[KEY_COUNT];
        MouseInput mouse;
        bool over_window = false;
        bool window_focused = false;
        bool any_key_pressed = false;
        float padding[2];

        UserInput() = default;
        UserInput(const UserInput& other)
        {
            memcpy(keyboard, other.keyboard, sizeof(keyboard));
            mouse = other.mouse;
            over_window = other.over_window;
            window_focused = other.window_focused;
            any_key_pressed = other.any_key_pressed;
            padding[0] = other.padding[0];
            padding[1] = other.padding[1];
        }

        UserInput& operator=(const UserInput& other)
        {
            if (this != &other)
            {
                memcpy(keyboard, other.keyboard, sizeof(keyboard));
                mouse = other.mouse;
                over_window = other.over_window;
                window_focused = other.window_focused;
                any_key_pressed = other.any_key_pressed;
                padding[0] = other.padding[0];
                padding[1] = other.padding[1];
            }
            return *this;
        }
    };

    struct ERA_CORE_API InputConfig
    {
        // Mouse settings
        float mouse_sensitivity = 0.15f;
        float scroll_sensitivity = 0.5f;
        bool invert_y = false;
        bool use_raw_input = true;

        // Mouse behavior
        bool auto_capture_on_focus = true;
        bool right_click_to_capture = true;
        bool show_cursor_after_idle = true;
        float idle_time_before_show = 3.0f;

        // Key bindings
        uint32 key_toggle_mouse = KEY_ESC;
        uint32 key_toggle_cursor = KEY_TAB;
        uint32 key_capture = KEY_RIGHT_MOUSE;

        // Camera settings
        float camera_distance = 5.0f;
        float camera_min_distance = 2.0f;
        float camera_max_distance = 15.0f;
        float camera_smooth_speed = 5.0f;
        float camera_pitch_limit = 89.0f;
    };

    ERA_CORE_API const UserInput& get_current_frame_input();
}