using System.Runtime.InteropServices;

namespace EraEngine.Runtime;

public class ComponentHandler
{
    [UnmanagedCaller]
    public static void AddComponent(int id, IntPtr comp)
    {
        try
        {
            string result = Marshal.PtrToStringAnsi(comp);
            var entity = Scene.GetEntity(id);
            var c = ScriptingCore.UserScriptingObj.GetComponent(result);
            entity.CopyComponent(c);
        }
        catch(Exception e) { Console.WriteLine(e.Message); }

    }

    [UnmanagedCaller]
    public static void RemoveComponent(int id, IntPtr comp)
    {
        string result = Marshal.PtrToStringAnsi(comp);
        var entity = Scene.GetEntity(id);
        entity.RemoveComponent(result);
    }
}
