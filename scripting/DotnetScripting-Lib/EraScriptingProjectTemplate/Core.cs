using EraScriptingCore;
using EraScriptingCore.Core;
using EraScriptingCore.Domain;
using EraScriptingCore.Domain.Components;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using EraScriptingCore.Infrastructure.Serializers;
using RGiesecke.DllExport;

namespace EraScriptingProjectTemplate;

public static class Core
{
    internal static Dictionary<string, EComponentCreationHelper.CreateComponentFunc> Types = new();

    private static void RegisterClasses() 
    {
        Types.Add("TestScript", () => new TestScript());
    }

    [UnmanagedCallersOnly(EntryPoint = "SerializeUserTypes")]
    public static unsafe void SerializeUserTypes()
    {
        var types = Types.Keys.ToList();
        var strTypes = "";
        var count = types.Count;
        for (int i = 0; i < count; i++)
        {
            strTypes += types[i];
            if (i < count - 1)
                strTypes += " ";
        }
        strTypes += Environment.NewLine;

        string path = @"bin\Release_x86_64\types.cfg";
        if (!File.Exists(path))
            File.WriteAllText(path, strTypes);
    }

    [UnmanagedCallersOnly(EntryPoint = "CreateTestScript")]
    public static IntPtr CreateTestScript() 
    {
        TestScript ts = new();
        GCHandle handle = GCHandle.Alloc(ts);
        IntPtr parameter = (IntPtr)handle;
        
        return parameter;
    }

    [UnmanagedCallersOnly(EntryPoint = "GetTypes")]
    public static unsafe IntPtr GetUserTypes() 
    {
        var types = Types.Keys.ToList();
        var strTypes = "";
        var count = types.Count;
        for(int i= 0; i < count; i++)
        {
            strTypes += types[i];
            if (i < count - 1)
                strTypes += " ";
        }

        var length = Encoding.ASCII.GetByteCount(strTypes);

        var bufferPtr = Marshal.AllocHGlobal(length + 1);

        var buffer = new Span<byte>(bufferPtr.ToPointer(), length);
        Encoding.ASCII.GetBytes(strTypes, buffer);

        Marshal.WriteByte(bufferPtr, length, 0);

        return bufferPtr;
    }

    [UnmanagedCallersOnly(EntryPoint = "Init")]
    public static void Init() 
    {
        RegisterClasses();
        foreach (var type in Types)
        {
            Console.WriteLine("Scripting> Type found: " + type.Key);
        }
    }
}
