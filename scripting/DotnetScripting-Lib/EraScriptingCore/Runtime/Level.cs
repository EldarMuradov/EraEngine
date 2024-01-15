using EraEngine.Components;

namespace EraEngine;

public static class Level
{
    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    internal delegate void UpdateDelegates();
    internal static UpdateDelegates UpdateDelegate;

    [UnmanagedCaller]
    public static unsafe void Start()
    {
        EEntity nav_target = new(59, "SpherePX1");
        nav_target.CreateComponent<RigidbodyComponent>(RigidbodyType.Dynamic);

        EEntity e = new(60, "SpherePX");
        e.CreateComponent<RigidbodyComponent>(RigidbodyType.Dynamic);

        Scene.SyncEntities();

        _syncObj.Wait();
        var values = Scene.Entities.Values;
        foreach (var entity in values)
            entity.Start();
        Debug.Log("Start");
        _syncObj.Release();
    }

    [UnmanagedCaller]
    public static void Update(float dt)
    {
        _syncObj.Wait();

        UpdateDelegate?.Invoke();

        var values = Scene.Entities.Values;
        foreach (var entity in values)
            entity.Update(dt);
        Debug.Log("Update");
        Scene.SyncEntities();
        _syncObj.Release();
    }
}
