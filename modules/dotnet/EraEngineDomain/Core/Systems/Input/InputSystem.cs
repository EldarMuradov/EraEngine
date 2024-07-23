using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace EraEngine;

public enum KeyCode
{
    Shift = 0x10,
    Ctrl = 0x11,
    Alt = 0x12,
    Esc = 0x1B,
    Caps = 0x14,
    Space = 0x20,
    Enter = 0x0D,
    Backspace = 0x08,
    Delete = 0x2E,
    Tab = 0x09,
    Left = 0x25,
    Right = 0x27,
    Up = 0x26,
    Down = 0x28,
    Print = 0x2C,
    F1 = 0x70,
    F2 = 0x71,
    F3 = 0x72,
    F4 = 0x73,
    F5 = 0x74,
    F6 = 0x75,
    F7 = 0x76,
    F8 = 0x77,
    F9 = 0x78,
    F10 = 0x79,
    F11 = 0x7A,
    F12 = 0x7B
};

[StructLayout(LayoutKind.Sequential)]
public record struct InputKey(bool Down, bool PressEvent);

[StructLayout(LayoutKind.Sequential)]
public record struct InputMouseButton(bool Down, bool ClickEvent,
    bool DoubleClickEvent, bool Padding);

[StructLayout(LayoutKind.Sequential)]
public record struct MouseInput(
    InputMouseButton Left,
    InputMouseButton Right,
    InputMouseButton Middle,

    float Scroll,

    int X,
    int Y,

    int Dx,
    int Dy,

    float RelX,
    float RelY,

    float Reldx,
    float Reldy
    );

[StructLayout(LayoutKind.Sequential)]
public record struct Input
{
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 128)]
    public InputKey[] Keyboard; // 128

    public MouseInput Mouse;

    public bool OverWindow;

    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public bool[] Padding; // 3
};

public class InputSystem : IESystem
{
    public Input Input
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private set;
    } = new();

    public ESystemPriority Priority
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public InputSystem()
    {
        Priority = ESystemPriority.High;
    }

    public void Update(World world, float dt)
    {

    }

    public void HandleUserInput(in Input input)
    {
        Input = input;
    }
}