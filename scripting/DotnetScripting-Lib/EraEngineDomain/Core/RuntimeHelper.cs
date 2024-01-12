using EraEngine.Components;
using System.Runtime.InteropServices;

namespace EraEngine.Core;

public static class RuntimeHelper
{
    public delegate EComponent CreateComponentFunc();

    private static IntPtr _hModule = 0;

    public static Dictionary<string, CreateComponentFunc> ComponentCreationFuncs = new();

    public static bool LoadUserLibrary(string dllPath)
    {
        _hModule = LoadLibrary(dllPath);
        if (_hModule != 0)
            return true;
        return false;
    }

    public static bool FreeUserLibrary()
    {
        return FreeLibrary(_hModule);
    }

    public static Result<CreateComponentFunc> LoadScriptCreationFunc(string functionName, string componentName)
    {
        var functionAddress = GetProcAddress(_hModule, functionName);
        var func = Marshal.GetDelegateForFunctionPointer<CreateComponentFunc>(functionAddress);
        if (!ComponentCreationFuncs.TryAdd(componentName, func))
            return Result<CreateComponentFunc>.Failure("Failed to add function to registry!");
        return Result<CreateComponentFunc>.Success(func);
    }

    public static void CallFunction(string functionName)
    {
        var functionAddress = GetProcAddress(_hModule, functionName);
        var func = Marshal.GetDelegateForFunctionPointer<Action>(functionAddress);
        func();
    }

    [DllImport("Kernel32.dll")]
    private static extern IntPtr LoadLibrary(string path);

    [DllImport("Kernel32.dll")]
    private static extern bool FreeLibrary(IntPtr lib);

    [DllImport("Kernel32.dll")]
    private static extern IntPtr GetProcAddress(IntPtr hModule, string procName);
}
