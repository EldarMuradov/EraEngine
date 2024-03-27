using System.Runtime.InteropServices;
namespace EraEngine;

public enum LogLevel : byte
{
    Normal,
    Warning,
    Error
}

public static class Debug
{
    public static void Log(string message, LogLevel level = LogLevel.Normal)
    {
        log_message((byte)level, message);
    }

    public static void LogError(string message)
    {
        log_message((byte)LogLevel.Error, message);
    }

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern unsafe void log_message(byte mode, string message);
}
