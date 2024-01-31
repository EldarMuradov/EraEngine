using EraEngine.Configuration;
using EraEngine.Core;
using EraEngine.Extensions;

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

    private static async Task InitAsync()
    {
        using StreamReader sr = new(Path.Combine(Environment.CurrentDirectory, "bin", "Release_x86_64", "core.cfg"));
        var res = await sr.ReadToEndAsync();
        var str = res.Split(',');
        Project.Name = str[0];
        Project.Path = str[1];
        InitializeScripting();
    }

    #region Invoke from C++

    [UnmanagedCaller]
    public static void MainFunc()
    {
        Debug.Log("Initializing scripting");
        InitAsync().FireAndForget((Exception ex) => Console.WriteLine(ex.Message));
    }

    [UnmanagedCaller]
    public static void InitializeScripting()
    {
        Debug.Log("Loading user dll");
        UserScriptingObj.LoadDll();
        ELevel.RegisterSystemsWithReflection();
    }

    [UnmanagedCaller]
    public static void ShutdownScripting()
    {
        ESystemManager.ReleaseSystems();
    }

    [UnmanagedCaller]
    public static void ReloadScripting()
    {
        try
        {
            Console.WriteLine("Reloading user dll");
            ESystemManager.ReleaseSystems();
            UserScriptingObj.ReloadScripting();
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.Message);
        }
    }

    #endregion
}
