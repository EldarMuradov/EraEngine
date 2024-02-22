using System.Runtime.InteropServices;

namespace EraEngine.Runtime;

public class ComponentHandler : IEHandler
{
    [UnmanagedCaller]
    public static void AddComponent(int id, IntPtr comp)
    {
        try
        {
            string result = Marshal.PtrToStringAnsi(comp);
            var entity = EWorld.SceneWorld.GetEntity(id);
            var c = ScriptingCore.UserScriptingObj.GetComponent(result);
            entity.CopyComponent(c);
        }
        catch(Exception e) { Console.WriteLine(e.Message); }

    }

    [UnmanagedCaller]
    public static void RemoveComponent(int id, IntPtr comp)
    {
        try
        {
            string result = Marshal.PtrToStringAnsi(comp);
            var entity = EWorld.SceneWorld.GetEntity(id);
            entity.RemoveComponent(result);
        }
        catch (Exception e) { Console.WriteLine(e.Message); }
    }
}
