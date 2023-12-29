using EraEngine.Components;
using System.Runtime.InteropServices;

namespace EraEngine;

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
