using EraScriptingCore;
using EraScriptingCore.Domain;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json.Serialization;

namespace EraScriptingProjectTemplate;

[JsonSerializable(typeof(string), GenerationMode = JsonSourceGenerationMode.Metadata)]
internal partial class JsonContext : JsonSerializerContext { }

public static class Core
{
    internal static Dictionary<string, Func<Script>> Types = new();

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

    public static CustomTypesDTO GetCtors()
    {
        return new CustomTypesDTO() { CustomTypes = Types };
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
