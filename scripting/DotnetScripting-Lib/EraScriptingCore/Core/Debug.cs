using System.Runtime.InteropServices;
namespace EraScriptingCore.Core;

public enum LogLevel : uint
{
    Normal,
    Warning,
    Error
}

public static class Debug
{
    public static void Log(string message, LogLevel level = LogLevel.Normal)
    {
        log_message((uint)level, message);
    }

    public static void LogError(string message)
    {
        log_message((uint)LogLevel.Error, message);
    }

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern unsafe void log_message(uint mode, string message);
}
