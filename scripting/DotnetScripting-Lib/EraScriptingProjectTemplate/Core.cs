using EraScriptingCore.Domain;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace EraScriptingProjectTemplate
{
    [JsonSerializable(typeof(string), GenerationMode = JsonSourceGenerationMode.Metadata)]
    internal partial class JsonContext : JsonSerializerContext { }

    public static class Core
    {
        internal static Dictionary<string, Func<Script>> Types = new();

        private static void RegisterClasses() 
        {
            Types.Add("TestScript", () => new TestScript());
        }

        [UnmanagedCallersOnly(EntryPoint = "GetTypes")]
        public static unsafe IntPtr GetUserTypes() 
        {
            var types = Types.Keys.ToList();
            var strTypes = "";
            foreach(var t in types)
                strTypes += t;

            var length = Encoding.UTF8.GetByteCount(strTypes);

            var bufferPtr = Marshal.AllocHGlobal(length + 1);

            var buffer = new Span<byte>(bufferPtr.ToPointer(), length);
            Encoding.UTF8.GetBytes(strTypes, buffer);

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
}
