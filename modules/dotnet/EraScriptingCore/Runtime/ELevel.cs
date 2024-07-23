using EraEngine.Components;

namespace EraEngine;
public static class ELevel
{
    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    internal delegate void UpdateDelegates(float dt);
    internal static UpdateDelegates? UpdateDelegate;

    [UnmanagedCaller]
    public static void Start()
    {
        try
        {
            {
                // Test case
                BitMask include = new();
                include.Set(ComponentMeta<TransformComponent>.Id);

                BitMask exclude = new();
                //exclude.Set(ComponentMeta<RigidbodyComponent>.Id);

                var filterId = EWorld.SceneWorld.RegisterFilter(include, exclude);

                EEntity nav_target = new(59, "SpherePX1");
                nav_target.CreateComponent<RigidbodyComponent>(RigidbodyType.Dynamic);

                EEntity.Instantiate(nav_target);

                EEntity e = new(60, "SpherePX");
                e.CreateComponent<RigidbodyComponent>(RigidbodyType.Dynamic);

                var filter = EWorld.SceneWorld.GetFilter(filterId);

                EWorld.SceneWorld.Iterate((entity) => { Console.WriteLine(entity.Id); }, filter);

                EWorld.SceneWorld.SyncEntities();
            }

            _syncObj.Wait();

            EWorld.SceneWorld.IterateAll((entity) => { entity.Start(); });

            _syncObj.Release();

            // Channel tests
            {
                ESystemManager.GetSystem<BackgroundServiceSystem>().QueueTask(async () =>
                {
                    var eventChannel = ESystemManager.GetSystem<EventSystem>().Channel;

                    var subscribe = (EventRequest req) => { Console.WriteLine(req.Name); };
                    var unsubscribe = () => { Console.WriteLine("Completed"); };

                    {
                        using var subscription = eventChannel.Subscribe(subscribe, unsubscribe);

                        eventChannel.Post(new EventRequest() { Name = "REQ1" });
                        eventChannel.Post(new EventRequest() { Name = "REQ2" });
                        eventChannel.Post(new EventRequest() { Name = "REQ3" });
                    }

                    eventChannel.Post(new EventRequest() { Name = "REQ4" });

                    await foreach (var req in eventChannel.ToAsyncEnumerable(CancellationToken.None))
                    {
                        await Console.Out.WriteLineAsync(req.Name + " " + req.IsCompleted);
                    }
                }).ConfigureAwait(false).GetAwaiter().GetResult();
            }
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

    private static void UpdateSystems(float dt)
    {
        var systems = ESystemManager.GetSystemsQueue();
        while (systems.TryDequeue(out var system, out _))
        {
            system.Update(EWorld.SceneWorld, dt);
        }
    }
}
