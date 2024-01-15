using EraEngine.Components;
using EraEngine.Configuration;
using EraEngine.FileSystem;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace EraEngine.Core;

class HostAssemblyLoadContext : AssemblyLoadContext
{
    private AssemblyDependencyResolver _resolver;

    public HostAssemblyLoadContext(string pluginPath) : base(isCollectible: true)
    {
        _resolver = new AssemblyDependencyResolver(pluginPath);
    }

    protected override Assembly Load(AssemblyName name)
    {
        string assemblyPath = _resolver.ResolveAssemblyToPath(name);
        if (assemblyPath != null)
        {
            Console.WriteLine($"Loading assembly {assemblyPath} into the HostAssemblyLoadContext");
            return LoadFromAssemblyPath(assemblyPath);
        }

        return null;
    }
}

public sealed class UserScripting
{
    private HostAssemblyLoadContext _context;

    private Assembly _assembly;

    internal void LoadDll()
    {
        if (!Directory.Exists(Project.TempDllPath))
        {
            Directory.CreateDirectory(Project.Path + "\\" + Project.TempDllPath);
            Debug.Log("Created temp path");
        }

        Debug.Log(Project.Path);
        Debug.Log(Project.Name);
        Debug.Log(Path.Combine(Project.Path, "assets\\scripts", Project.Name, Project.Name, "bin\\Release\\net8.0"));
        FileCloner fileCloner = new();
        fileCloner.Clone(Path.Combine(Project.Path, "assets\\scripts", Project.Name, Project.Name, "bin\\Release\\net8.0"), Project.Path + "\\" + Project.TempDllPath);
        Debug.Log("Cloned all files");

        _context = new HostAssemblyLoadContext(Project.Path + "\\" + Project.TempDllPath + $"\\{Project.Name}.dll");

        _assembly = _context.LoadFromAssemblyPath(Project.Path + "\\" + Project.TempDllPath + $"\\{Project.Name}.dll");

        Debug.Log("Assembly loaded successfuly");

        var types = _assembly.GetTypes();
        string[] names = new string[types.Length];
        int i = 0, length = types.Length;
        for (; i < length; i++)
        {
            names[i] = types[i].Name;
            sendType(names[i]);
        }

        Debug.Log("All types are sent");
    }

    internal void UnloadDll()
    {
        Console.WriteLine("1");
        _context.Unload();
        Console.WriteLine("2");
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
        Console.WriteLine("3");
    }

    internal EComponent GetComponent(string name)
    {
        var type = _assembly.GetType(Project.Name + "." + name);
        if (type != null)
        {
            
            var component = Unsafe.As<EComponent>(Activator.CreateInstance(type));

            return component;
        }

        return null!;
    }

    internal void ReloadScripting()
    {
        try
        {
            UnloadDll();
            Console.WriteLine("4");

            LoadDll();
        }
        catch (Exception e) { Console.WriteLine(e.Message); }
    }

    private void OnUnloaded(AssemblyLoadContext ctx) { Debug.Log("Assembly unloaded."); }

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private extern static void sendType(string types);
}
