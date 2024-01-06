using System.Runtime.InteropServices;
using EraScriptingProjectTemplate;

namespace EraEngine;

public static class Level
{
    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    [UnmanagedCallersOnly(EntryPoint = "start")]
    public static unsafe void Start()
    {
        EEntity nav_target = new(3, "Sphere");

        EEntity e = new(4, "SphereNav");
        e.CreateComponent<TestScript>();

        Scene.SyncEntities();

        _syncObj.Wait();
        var values = Scene.Entities.Values;
        foreach (var entity in values)
            entity.Start();
        _syncObj.Release();
    }

    [UnmanagedCallersOnly(EntryPoint = "update")]
    public static void Update(float dt)
    {
        _syncObj.Wait();
        var values = Scene.Entities.Values;
        foreach (var entity in values)
            entity.Update(dt);
        Scene.SyncEntities();
        _syncObj.Release();
    }
}
