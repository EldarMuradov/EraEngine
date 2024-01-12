using EraEngine.Core;
using System.Runtime.InteropServices;

namespace EraEngine;

public class ScriptingCore
{
    #region P/I

    [UnmanagedCallersOnly(EntryPoint = "init_scripting")]
    public static unsafe void InitializeScripting()
    {
        Debug.Log("Usage of debug");

        RuntimeHelper.LoadUserLibrary("EraScriptingProjectTemplate.dll");
        RuntimeHelper.CallFunction("InitializeScripting");
    }

    #endregion
}
