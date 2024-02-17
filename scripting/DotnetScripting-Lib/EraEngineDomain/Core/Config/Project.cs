namespace EraEngine.Configuration;

public enum ConfigType
{
    None,
    Release,
    Debug
}

public static class Project
{
    public static string Name = "";
    public static string Path = "";
    public static string TempDllPath = "Temp";
    public static ConfigType Configuration = ConfigType.Release;
    public static Version Version = new Version(0, 0);
}