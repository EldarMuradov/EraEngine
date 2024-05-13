using System.Runtime.CompilerServices;

namespace EraEngine;

public class BackgroundServiceSystem : IESystem
{
    public ESystemPriority Priority
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public BackgroundServiceSystem()
    {
        Priority = ESystemPriority.Low;
    }

    public void Update(World world, float dt)
    {
    }

    public void QueueJob(Action request)
    {
        if (!ThreadPool.QueueUserWorkItem((object? state) => { request(); }))
            Debug.LogError("Scripting> Failed to queue action in thread pool!");
    }

    public Task QueueTask(Action request)
    {
        return Task.Factory.StartNew(() => { request(); });
    }
}