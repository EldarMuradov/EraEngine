using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace EraScriptingCore;

[JsonSerializable(typeof(string), GenerationMode = JsonSourceGenerationMode.Metadata)]
internal partial class JsonContext : JsonSerializerContext { }

public class ScriptingCore
{
    private static int _typesLength = 0;

    [DllImport("EraScriptingProjectTemplate.dll")]
    public static extern IntPtr GetTypes();

    [DllImport("EraScriptingProjectTemplate.dll")]
    public static extern void Init();

    [UnmanagedCallersOnly(EntryPoint = "init_scripting")]
    public static void InitializeScripting() 
    {
        Console.WriteLine("Hello from C#!");

        Init();

        IntPtr ptr = GetTypes();
        int length = GetStringLength(ptr);
        _typesLength = length;
    }

    [UnmanagedCallersOnly(EntryPoint = "get_types_length")]
    private static unsafe int GetTypesLength()
    {
        return _typesLength;
    }

    [UnmanagedCallersOnly(EntryPoint = "get_types")]
    public static unsafe IntPtr GetAssemblyTypes()
    {
        IntPtr ptr = GetTypes();

        var serializedData = new Span<byte>((void*)ptr, _typesLength);

        var strings = "";
        Encoding.UTF8.TryGetChars(serializedData, strings.ToCharArray(), out int i);

        var bufferPtr = Marshal.AllocHGlobal(_typesLength + 1);
        Marshal.Copy(strings.ToCharArray(), 0, bufferPtr, _typesLength + 1);
        //var buffer = new Span<byte>(bufferPtr.ToPointer(), length);
        //Encoding.UTF8.GetBytes(strings, buffer);

        Marshal.WriteByte(bufferPtr, _typesLength, 0);

        return bufferPtr;
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
