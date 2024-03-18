using System.Collections.Immutable;
using System.Reflection;
using EraEngine.Configuration;
using Microsoft.Extensions.Configuration;

namespace EraEngine.Core;

internal static class DependencyResolver
{
    public static void RegisterCommonSystems()
    {
        ESystemManager.RegisterSystem<BackgroundServiceSystem>();
        ESystemManager.RegisterSystem<InputSystem>();
        ESystemManager.RegisterSystem<EventSystem>();
    }
    private static bool IsSystemType(Type type) => type.GetInterface("IESystem") is not null && type.IsClass;

    public static void RegisterAllSystemsWithReflection() 
    {
        var assembly = Assembly.GetAssembly(typeof(EEntity));
        var types = assembly.GetTypes().Where(IsSystemType).ToImmutableList();

        foreach (var type in types)
        {
            ESystemManager.RegisterSystem((IESystem)Activator.CreateInstance(type)!);
            Console.WriteLine(type.Name);
        }
    }

    public static void RegisterSystemsWithReflection()
    {
        try
        {
            var config = new ConfigurationBuilder()
            .AddJsonFile(Path.Combine(Directory.GetCurrentDirectory(), "scripting", "DotnetScripting-Lib", "EraScriptingCore", "systems_injection.json"))
            .Build();

            var registration = config.GetSection("registrations").GetChildren();

            var assembly = Assembly.GetAssembly(typeof(EEntity));

            foreach (var reg in registration)
            {
                var type = assembly.GetType(reg["system"]);

                if (IsSystemType(type))
                {
                    var instance = (IESystem)Activator.CreateInstance(type)!;
                    ESystemManager.RegisterSystem(instance);
                }
                else
                    Debug.LogError($"Scripting> Failed to register {type.Name} system. System should be derrived from IESystem.");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.Message);
        }
    }

    public static void RegisterUserSystemsWithReflection(in Assembly assembly)
    {
        try
        {
            var config = new ConfigurationBuilder()
            .AddJsonFile(Path.Combine(Project.Path, "assets", "scripts", Project.Name, Project.Name, "systems_injection.json"))
            .Build();

            var registration = config.GetSection("registrations").GetChildren();

            foreach (var reg in registration)
            {
                var type = assembly.GetType(reg["system"]);
                if (IsSystemType(type))
                {
                    var instance = (IESystem)Activator.CreateInstance(type)!;
                    ESystemManager.RegisterSystem(instance);
                }
                else
                    Debug.LogError($"Scripting> Failed to register {type.Name} system. System should be derrived from IESystem.");
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.Message);
        }
    }
}
