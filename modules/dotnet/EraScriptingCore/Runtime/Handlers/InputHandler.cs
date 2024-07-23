using EraEngine.Extensions;

namespace EraEngine.Core;

public static class InputHandler
{
    [UnmanagedCaller]
    public static unsafe void HandleInput(IntPtr inputPtr)
    {
        try
        {
            void* ptr = inputPtr.ToPointer();

            InputKey[] keys = Memory.Create<InputKey>(ptr, 128);
            Input input = new()
            {
                Keyboard = keys
            };

            ESystemManager.GetSystem<InputSystem>()?.HandleUserInput(input);
        }
        catch (Exception e)
        {
            Console.WriteLine(e.Message);
            Debug.LogError(e.Message);
        }
    }
}
