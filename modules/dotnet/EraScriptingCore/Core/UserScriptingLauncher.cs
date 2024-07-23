using EraEngine.Configuration;
using EraEngine.FileSystem;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace EraEngine.Core;

public class HostAssemblyLoadContext : AssemblyLoadContext
{
    private readonly AssemblyDependencyResolver _resolver;

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
        try
        {
            _userTypes.Clear();
            _userTypes = [];

            Parallel.ForEach(EWorld.SceneWorld.Entities.Values, (e) =>
            {
                foreach (var comp in e.Components.Values)
                {
                    if (comp is EScript)
                    {
                        var name = comp.GetType().Name;
                        e.Components.TempCompData.Add(name);
                        e.RemoveComponent(name, false);
                    }
                }
            });

            bool unloading = true;
            context.Unloading += (alc) => { unloading = false; };
            context.Unload();

            while (unloading);

            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
        }
        catch (Exception e)
        {
            Console.WriteLine(e.Message);
            Debug.LogError(e.Message);
        }
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
            if(type.IsSubclassOf(typeof(EScript)))
                sendType(type.Name);
        }

        try
        {
            DependencyResolver.RegisterUserSystemsWithReflection(assembly);

            EWorld.SceneWorld.RefreshScene();

            Parallel.ForEach(EWorld.SceneWorld.Entities.Values, (e) =>
            {
                try
                {
                    if (e is null)
                        return;

                    if (e.Components?.TempCompData?.Count > 0)
                    {
                        foreach (var data in e.Components.TempCompData)
                        {
                            e.CopyComponent(GetComponent(data));
                        }
                        e.Components.TempCompData.Clear();
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.Message);
                }
            });
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.Message);
        }
        finally
        {
            bool unloading = true;
            _alc.Unloading += (alc) => { unloading = false; };
            _alc.Unload();

            while (unloading) ;

            _alc = new(assemblyPath);
        }
    }

    internal void LoadDll()
    {
        try
        {
            string projectDllPath = Path.Combine(Project.Path, "resources", "assets", "scripts", Project.Name, Project.Name, "bin", "Release", "net8.0");
            string tempPath = Path.Combine(Project.Path, Project.TempDllPath);
            string tempDllPath = Path.Combine(Project.Path, Project.TempDllPath, $"{Project.Name}.dll");

            if (!Directory.Exists(Project.TempDllPath))
            {
                Directory.CreateDirectory(tempPath);
                Debug.Log("Created temp path");
            }

            if (_alc is not null)
            {
                WeakReference alcWeakReference = new(_alc);
                UnloadAssembly(_alc);
                _alc = new(tempDllPath);

                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();

                Debug.Log($"Unload user dll success: {!alcWeakReference.IsAlive}");
            }

            Debug.Log("Before cleaning");
            FileCloner fileCloner = new();
            fileCloner.Clone(projectDllPath, tempPath);
            Debug.Log("Cloned all files");
            Debug.Log(tempDllPath);
            ExecuteAndUnload(tempDllPath, out WeakReference hostAlcWeakRef);
            
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            Debug.Log($"Load user dll success: {hostAlcWeakRef.IsAlive}");
        }
        catch (Exception e)
        {
            Console.WriteLine(e.Message);
            Debug.Log(e.Message);
        }
    }

    internal EComponent? GetComponent(string name)
    {
        if(_userTypes.TryGetValue(name, out var type))
            return Unsafe.As<EComponent>(Activator.CreateInstance(type));

        Debug.LogError("Failed to get a type!");
        return null;
    }

    internal void ReloadScripting()
    {
        try
        {
            LoadDll();
        }
        catch (Exception e) 
        { 
            Console.WriteLine(e.Message);
            Debug.LogError(e.Message);
        }
    }

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private extern static void sendType(string types);
}
