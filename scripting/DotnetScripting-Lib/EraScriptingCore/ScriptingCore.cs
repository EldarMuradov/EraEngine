using EraEngine.Configuration;
using EraEngine.Core;

namespace EraEngine;

public delegate void Call();
public delegate Task CallMain();
public delegate void CallUpdate(float dt);
public delegate void CallHandleColls(int id1, int id2);
public delegate void CallHandleTrs(IntPtr trs, int id);

public delegate void CallAddComp(int id, IntPtr comp);
public delegate void CallRemoveComp(int id, IntPtr comp);

public class ScriptingCore
{
    internal static UserScriptingLauncher UserScriptingObj = new();

    public static void Main(string[] args)
    {

    }

    #region Invoke from C++

    [UnmanagedCaller]
    public static void MainFunc()
    {
        using StreamReader sr = new("E:\\Era Engine\\bin\\Release_x86_64\\core.cfg");
        var res = sr.ReadToEnd();
        var str = res.Split(',');
        Project.Name = str[0];
        Project.Path = str[1];
        InitializeScripting();
    }

    [UnmanagedCaller]
    public static void InitializeScripting()
    {
        Debug.Log("Loading user dll");
        UserScriptingObj.LoadDll();
    }

    [UnmanagedCaller]
    public static void ShutdownScripting()
    {
        //Debug.Log("Unloading user dll");
        //UserScriptingObj.UnloadAssembly();
    }

    [UnmanagedCaller]
    public static void ReloadScripting()
    {
        Console.WriteLine("Reloading user dll");
        UserScriptingObj.ReloadScripting();
    }

    #endregion
}
