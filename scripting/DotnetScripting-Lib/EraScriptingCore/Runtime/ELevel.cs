using EraEngine.Components;
using System.Collections.Immutable;
using System.Reflection;

namespace EraEngine;
public static class ELevel
{
    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    internal delegate void UpdateDelegates(float dt);
    internal static UpdateDelegates? UpdateDelegate;

    [UnmanagedCaller]
    public static unsafe void Start()
    {
        try
        {
            {
                // Test case
                BitMask include = new();
                include.Set(ComponentMeta<TransformComponent>.Id);

                BitMask exclude = new();
                exclude.Set(ComponentMeta<RigidbodyComponent>.Id);

                var filterId = EWorld.SceneWorld.RegisterFilter(include, exclude);

                EEntity nav_target = new(59, "SpherePX1");
                nav_target.CreateComponent<RigidbodyComponent>(RigidbodyType.Dynamic);

                EEntity e = new(60, "SpherePX");
                e.CreateComponent<RigidbodyComponent>(RigidbodyType.Dynamic);

                EEntity e1 = new();

                var filter = EWorld.SceneWorld.GetFilter(filterId);

                EWorld.SceneWorld.Iterate((entity) => { Console.WriteLine(entity.Id); }, filter);

                EWorld.SceneWorld.SyncEntities();
            }

            _syncObj.Wait();

            EWorld.SceneWorld.IterateAll((entity) => { entity.Start(); });

            _syncObj.Release();
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.Message);
        }
    }

    [UnmanagedCaller]
    public static void Update(float dt)
    {
        try
        {
            _syncObj.Wait();

            UpdateDelegate?.Invoke(dt);
            UpdateSystems(dt);

            EWorld.SceneWorld.IterateAll((entity) => { entity.Update(dt); });

            EWorld.SceneWorld.SyncEntities();

            _syncObj.Release();
        }
        catch (Exception ex)
        {
            Console.WriteLine(ex.Message);
        }
    }

    public static void RegisterCommonSystems()
    {
        ESystemManager.RegisterSystem<BackgroundServiceSystem>();
        ESystemManager.RegisterSystem<InputSystem>();
        ESystemManager.RegisterSystem<EventSystem>();
    }

    public static void RegisterSystemsWithReflection()
    {
        static bool IsSystemType(Type type) => type.GetInterface("IESystem") is not null && type.IsClass;

        var assembly = Assembly.GetAssembly(typeof(EEntity));
        var types = assembly.GetTypes().Where(IsSystemType).ToImmutableList();

        foreach (var type in types)
            ESystemManager.RegisterSystem((IESystem)Activator.CreateInstance(type)!);
    }

    private static void UpdateSystems(float dt)
    {
        var systems = ESystemManager.GetSystemsQueue();
        while (systems.TryDequeue(out var system, out _))
        {
            system.Update(EWorld.SceneWorld, dt);
        }
    }
}
