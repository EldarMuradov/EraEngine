using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text.Json.Serialization;

namespace EraScriptingCore;

[JsonSerializable(typeof(string), GenerationMode = JsonSourceGenerationMode.Metadata)]
internal partial class JsonContext : JsonSerializerContext { }

public class ScriptingCore
{
    [DllImport("EraScriptingProjectTemplate.dll")]
    public static extern IntPtr GetTypes();

    [DllImport("EraScriptingProjectTemplate.dll")]
    public static extern void SerializeUserTypes();

    [DllImport("EraScriptingProjectTemplate.dll")]
    public static extern void Init();

    [UnmanagedCallersOnly(EntryPoint = "init_scripting")]
    public static void InitializeScripting() 
    {
        Init();

        SerializeUserTypes();

        Core.Debug.Log("Usage of debug");
    }

    private static unsafe int GetStringLength(IntPtr str)
    {
        var ptr = (byte*)str;
        var length = 0;
        while (*(ptr + length) != 0)
            length++;
        return length;
    }
}
