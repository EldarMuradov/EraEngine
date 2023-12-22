using System.Runtime.InteropServices;
using EraScriptingCore.Core;
using EraEngineDomain.Domain;
using EraScriptingProjectTemplate;

namespace EraScriptingCore.Domain;

public static class Scene
{
    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    [UnmanagedCallersOnly(EntryPoint = "start")]
    public static unsafe void Start()
    {
        EEntity e = new(60, "Px_Sphere");
        Debug.Log(SceneDomain.Entities.Count.ToString());
        Console.WriteLine("User script");
        e.CreateComponent<TestScript>();
        _syncObj.Wait();
        var values = SceneDomain.Entities.Values;
        foreach (var entity in values)
            entity.Start();
        _syncObj.Release();
    }

    [UnmanagedCallersOnly(EntryPoint = "update")]
    public static void Update(float dt)
    {
        _syncObj.Wait();
        var values = SceneDomain.Entities.Values;
        foreach (var entity in values)
            entity.Update(dt);

        _syncObj.Release();
    }
}
