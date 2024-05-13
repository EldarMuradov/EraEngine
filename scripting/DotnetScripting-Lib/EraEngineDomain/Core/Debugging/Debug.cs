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
        logMessage((byte)level, message);
    }

    public static void LogError(string message)
    {
        logMessage((byte)LogLevel.Error, message);
    }

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern unsafe void logMessage(byte mode, string message);
}
