using System.Runtime.InteropServices;

namespace EraEngine;

[StructLayout(LayoutKind.Sequential)]
public struct EventRequest
{
    public bool IsCompleted;

    public string Name;
}

public class EventSystem : IESystem, IEHandler<EventRequest>
{
    public ESystemPriority Priority { get; }

    private Dictionary<string, EventRequest> _events = [];

    private readonly SemaphoreSlim _sync = new(1, 1);

    public EventSystem()
    {
        Priority = ESystemPriority.Normal;
    }

    public void Handle(EventRequest request)
    {
        _sync.Wait();
        _events.TryAdd(request.Name, request);
        _sync.Release();
    }

    public bool Signaled(string signal)
    {
        return _events.TryGetValue(signal, out _);
    }

    public void Update(float dt)
    {
    }
}
