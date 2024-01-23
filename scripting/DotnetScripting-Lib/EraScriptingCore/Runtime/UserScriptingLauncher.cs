using EraEngine.Components;
using EraEngine.Configuration;
using EraEngine.FileSystem;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace EraEngine.Core;

public class HostAssemblyLoadContext : AssemblyLoadContext
{
    private AssemblyDependencyResolver _resolver;

    public HostAssemblyLoadContext(string pluginPath) : base(isCollectible: true)
    {
        _resolver = new(pluginPath);
    }

    protected override Assembly Load(AssemblyName name)
    {
        string assemblyPath = _resolver.ResolveAssemblyToPath(name);
        if (assemblyPath != null && !assemblyPath.EndsWith("EraEngineDomain.dll"))
        {
            Console.WriteLine($"Loading assembly {assemblyPath} into the HostAssemblyLoadContext");
            return LoadFromAssemblyPath(assemblyPath);
        }

        return null!;
    }
}

public sealed class UserScriptingLauncher
{
    private Dictionary<string, Type> _userTypes = [];
    private HostAssemblyLoadContext _alc;

    internal void UnloadAssembly(HostAssemblyLoadContext context)
    {
        _userTypes.Clear();
        _userTypes = new();

        Parallel.ForEach(EWorld.Entities.Values, (e) => 
        {
            foreach (var comp in e.Components.Values)
            {
                if (comp is Script)
                {
                    var name = comp.GetType().Name;
                    e.Components.TempCompData.Add(name);
                    e.RemoveComponent(name, false);
                }
            }
        });

        bool unloading = true;
        context.Unloading += (alc) => { unloading = false; Console.WriteLine("Unloaded"); };
        context.Unload();

        while (unloading);

        GC.Collect();
        GC.WaitForPendingFinalizers();
    }

    public void ExecuteAndUnload(string assemblyPath, out WeakReference alcWeakRef)
    {
        _alc = new(assemblyPath);

        alcWeakRef = new WeakReference(_alc);

        var assembly = _alc.LoadFromAssemblyPath(assemblyPath);

        if (_userTypes.Count > 0)
        {
            _userTypes.Clear();
            _userTypes = [];
        }

        foreach (var type in assembly.GetTypes())
        {
            _userTypes.Add(type.Name, type);
            if(type.IsSubclassOf(typeof(Script)))
                sendType(type.Name);
            if (type.GetInterface("IESystem") is not null)
                ESystemManager.RegisterSystem((IESystem)Activator.CreateInstance(type)!);
        }

        Parallel.ForEach(EWorld.Entities.Values, (e) =>
        {
            if (e.Components.TempCompData.Count > 0)
            {
                foreach (var data in e.Components.TempCompData)
                {
                    e.CopyComponent(GetComponent(data));
                }
                e.Components.TempCompData.Clear();
            }
        });

        bool unloading = true;
        _alc.Unloading += (alc) => { unloading = false; };
        _alc.Unload();

        while (unloading);

        _alc = new(assemblyPath);
    }

    internal void LoadDll()
    {
        string projectPath = Path.Combine(Project.Path, "assets", "scripts", Project.Name, Project.Name, "bin", "Release", "net8.0");
        string tempPath = Path.Combine(Project.Path, Project.TempDllPath);
        string tempDllPath = Path.Combine(Project.Path, Project.TempDllPath, $"{Project.Name}.dll");

        if (!Directory.Exists(Project.TempDllPath))
        {
            Directory.CreateDirectory(tempPath);
            Debug.Log("Created temp path");
        }

        if (_alc is not null)
        {
            UnloadAssembly(_alc);
            _alc = new(tempDllPath);

            for (int i = 0; (i < 10); i++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }
        }

        Debug.Log("Before cleaning");
        FileCloner fileCloner = new();

        try
        {
            fileCloner.Clone(projectPath, tempPath);
            Debug.Log("Cloned all files");
        }
        catch (Exception e)
        {
            Debug.Log(e.Message);
        }

        ExecuteAndUnload(tempDllPath, out WeakReference hostAlcWeakRef);

        for (int i = 0; hostAlcWeakRef.IsAlive && (i < 10); i++)
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        Console.WriteLine($"Unload success: {!hostAlcWeakRef.IsAlive}");
    }

    internal EComponent GetComponent(string name)
    {
        if(_userTypes.TryGetValue(name, out var type))
            return Unsafe.As<EComponent>(Activator.CreateInstance(type));
        Debug.LogError("Failed to get a type!");
        return null!;
    }

    internal void ReloadScripting()
    {
        try
        {
            LoadDll();
        }
        catch (Exception e) { Console.WriteLine(e.Message); }
    }

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private extern static void sendType(string types);
}
