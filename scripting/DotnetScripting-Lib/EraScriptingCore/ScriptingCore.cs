using EraEngine.Components;
using System.Runtime.InteropServices;

public class App
{
    private static byte isWaiting = 0;
    private static int s_CallCount = 0;
    private static ManualResetEvent mre = new ManualResetEvent(false);

    public static void Main(string[] args)
    {
        Console.WriteLine($"{nameof(App)} started - args = [ {string.Join(", ", args)} ]");
        isWaiting = 1;
        mre.WaitOne();
    }

    [UnmanagedCallersOnly]
    public static byte IsWaiting() => isWaiting;

    [UnmanagedCallersOnly]
    public static void Hello(IntPtr message)
    {
        Console.WriteLine($"Hello, world! from {nameof(App)} [count: {++s_CallCount}]");
        Console.WriteLine($"-- message: {Marshal.PtrToStringUTF8(message)}");
        if (s_CallCount >= 3)
        {
            Console.WriteLine("Signaling app to close");
            mre.Set();
        }
    }
}

namespace EraEngine
{
    public static class EComponentCreationHelper
    {
        public delegate EComponent CreateComponentFunc();

        [DllImport("Kernel32.dll")]
        private static extern IntPtr LoadLibrary(string path);

        [DllImport("Kernel32.dll")]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        public static Delegate LoadFunction<T>(string dllPath, string functionName)
        {
            var hModule = LoadLibrary(dllPath);
            var functionAddress = GetProcAddress(hModule, functionName);
            return Marshal.GetDelegateForFunctionPointer(functionAddress, typeof(T));
        }

        public static Dictionary<string, CreateComponentFunc> ComponentCreationFuncs = new();
    }

    public class ScriptingCore
    {
        #region P/I

        [UnmanagedCallersOnly(EntryPoint = "init_scripting")]
        public static unsafe void InitializeScripting()
        {
            Core.Debug.Log("Usage of debug");
        }

        #endregion
    }
}
