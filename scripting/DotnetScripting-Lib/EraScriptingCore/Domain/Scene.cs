using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using EraScriptingCore.Core;

namespace EraScriptingCore.Domain;

[StructLayout(LayoutKind.Sequential)]
public struct EntityCreationDTO
{
    public IntPtr Entity;
}

public static class Scene
{
    public static Dictionary<int, EEntity> Entities = new();

    internal static Dictionary<int, EEntity> Additional = new();

    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    [UnmanagedCallersOnly(EntryPoint = "AddEntity")]
    public static unsafe void AddEntity(EntityCreationDTO creationDTO)
    {
        EEntity entity = Unsafe.Read<EEntity>(creationDTO.Entity.ToPointer());
        Entities.TryAdd(entity.Id, entity);
    }

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

        _syncObj.Release();
    }
}
