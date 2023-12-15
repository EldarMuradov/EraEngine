using EraScriptingCore.Domain;
using System.Runtime.InteropServices;
using System.Text.Json.Serialization;

namespace EraScriptingCore;

[JsonSerializable(typeof(string), GenerationMode = JsonSourceGenerationMode.Metadata)]
internal partial class JsonContext : JsonSerializerContext { }

[StructLayout(LayoutKind.Sequential)]
public struct CustomTypesDTO
{
    public Dictionary<string, Func<Script>> CustomTypes;
}

public class ScriptingCore
{
    #region P/I

    [DllImport("EraScriptingProjectTemplate.dll")]
    private static extern IntPtr GetTypes();

    [DllImport("EraScriptingProjectTemplate.dll")]
    private static extern void SerializeUserTypes();

    [DllImport("EraScriptingProjectTemplate.dll")]
    private static extern void Init();

    [UnmanagedCallersOnly(EntryPoint = "init_scripting")]
    public static void InitializeScripting() 
    {
        Init();

        SerializeUserTypes();

        Core.Debug.Log("Usage of debug");
    }

    #endregion

    private static unsafe int GetStringLength(IntPtr str)
    {
        var ptr = (byte*)str;
        var length = 0;
        while (*(ptr + length) != 0)
            length++;
        return length;
    }
}
