using System.Runtime.InteropServices;
using EraScriptingCore.Core;

namespace EraScriptingCore.Domain;

public sealed class Scene
{
    public static Dictionary<int, EEntity> Entities = new();

    internal static Dictionary<int, EEntity> Additional = new();

    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    [UnmanagedCallersOnly(EntryPoint = "start")]
    public static void Start()
    {
        _syncObj.Wait();
        var values = Entities.Values;
        foreach (var entity in values)
            entity.Start();
        _syncObj.Release();
    }

    [UnmanagedCallersOnly(EntryPoint = "update")]
    public static void Update(float dt)
    {
        _syncObj.Wait();
        var values = Entities.Values;
        foreach (var entity in values)
            entity.Update(dt);

        Debug.Log($"Scene> Update with {1.0f / dt} fps");

        _syncObj.Release();
    }
}
