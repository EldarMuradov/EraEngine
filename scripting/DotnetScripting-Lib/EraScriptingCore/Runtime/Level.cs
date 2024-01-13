using System.Diagnostics.CodeAnalysis;
using System.Runtime.InteropServices;
using EraEngine.Components;
using EraEngine.Core;

namespace EraEngine;

public static class Level
{
    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    [RequiresUnreferencedCode("sefsefes")]
    [UnmanagedCallersOnly(EntryPoint = "start")]
    public static unsafe void Start()
    {
        EEntity nav_target = new(59, "SpherePX1");
        nav_target.CreateComponent<RigidbodyComponent>(RigidbodyType.Dynamic);

        EEntity e = new(60, "SpherePX");
        e.CreateComponent<RigidbodyComponent>(RigidbodyType.Dynamic);

        Scene.SyncEntities();

        Debug.Log("Reloading");

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
