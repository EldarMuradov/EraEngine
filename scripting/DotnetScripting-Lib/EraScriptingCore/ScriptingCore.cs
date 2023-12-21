using EraScriptingCore.Domain.Components;
using System.Runtime.InteropServices;

namespace EraScriptingCore;

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

    [DllImport("EraScriptingProjectTemplate.dll")]
    private static extern void SerializeUserTypes();

    [DllImport("EraScriptingProjectTemplate.dll")]
    private static extern void Init();

    [UnmanagedCallersOnly(EntryPoint = "init_scripting")]
    public static unsafe void InitializeScripting()
    {
        Init();

        SerializeUserTypes();

        Core.Debug.Log("Usage of debug");
    }

    #endregion
}
