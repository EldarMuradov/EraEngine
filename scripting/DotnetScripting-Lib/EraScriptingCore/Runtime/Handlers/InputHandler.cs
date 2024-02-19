using EraEngine.Extensions;

namespace EraEngine.Core;

public static class InputHandler
{
    [UnmanagedCaller]
    public static unsafe void HandleInput(IntPtr inputPtr)
    {
        void* ptr = inputPtr.ToPointer();

        InputKey[] keys = Memory.Create<InputKey>(ptr, 128);
        Input input = new();
        input.Keyboard = keys;
        ESystemManager.GetSystem<InputSystem>().HandleUserInput(input);
    }
}
