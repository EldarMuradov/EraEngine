using System.Runtime.InteropServices;
namespace EraScriptingCore.Core;

public enum LogMessageMode : uint
{
    Normal,
    Warning,
    Error,
}

public static class Debug
{
    public static void Log(string message) 
    {
        log_message((uint)LogMessageMode.Normal, message);
    }

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern unsafe void log_message(uint mode, string message);
}
