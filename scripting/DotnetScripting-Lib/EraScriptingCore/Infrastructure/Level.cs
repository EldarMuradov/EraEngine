using System.Runtime.InteropServices;
using EraScriptingProjectTemplate;

namespace EraEngine;

public static class Level
{
    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    [UnmanagedCallersOnly(EntryPoint = "start")]
    public static unsafe void Start()
    {
        EEntity e = new(60, "Px_Sphere");
        e.CreateComponent<TestScript>();
        _syncObj.Wait();
        var values = new List<EEntity>(Scene.Entities.Values);
        foreach (var entity in values)
            entity.Start();
        _syncObj.Release();
    }

    [UnmanagedCallersOnly(EntryPoint = "update")]
    public static void Update(float dt)
    {
        _syncObj.Wait();
        var values = new List<EEntity>(Scene.Entities.Values);
        foreach (var entity in values)
            entity.Update(dt);

        _syncObj.Release();
    }
}
