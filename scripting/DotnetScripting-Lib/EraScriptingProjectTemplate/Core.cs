using EraScriptingCore.Domain;
using System.Runtime.InteropServices;

namespace EraScriptingProjectTemplate
{
    public static class Core
    {
        internal static Dictionary<string, Func<Script>> Types = new();

        private static void RegisterClasses() 
        {
            Types.Add("TestScript", () => new TestScript());
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
