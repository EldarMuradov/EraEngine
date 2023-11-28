using System.Runtime.InteropServices;

namespace EraScriptingCore;

public class ScriptingCore
{
    [DllImport("EraScriptingProjectTemplate.dll")]
    public static extern void Init();

    [UnmanagedCallersOnly(EntryPoint = "init_scripting")]
    public static void InitializeScripting() 
    {
        Console.WriteLine("Hello from C#!");

        Init();
    }
}
