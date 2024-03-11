using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading.Channels;

namespace EraEngine;

[StructLayout(LayoutKind.Sequential)]
public struct EventRequest
{
    public bool IsCompleted;

    public string Name;
}

public class EventChannel<T> : IDisposable
{
    private readonly Channel<T> _channel = Channel.CreateUnbounded<T>();

    public Task Completion => _channel.Reader.Completion;

    public bool Post(T item)
    {
        return _channel.Writer.TryWrite(item);
    }

    public IAsyncEnumerable<T> ToAsyncEnumerable(CancellationToken token)
    {
        return _channel.Reader.ReadAllAsync(token);
    }

    internal struct EventSubscription<TEventHandler> : IDisposable
        where TEventHandler : Delegate
    {
        private readonly Action _unsubscribe;

        public EventSubscription(
            TEventHandler handler,
            Action<TEventHandler> subscribe,
            Action<TEventHandler> unsubscribe)
        {
            subscribe(handler);
            _unsubscribe = () => unsubscribe(handler);
        }

        public void Dispose()
        {
            _unsubscribe();
        }
    }

    public IDisposable Subscribe<TEventHandler>(
        TEventHandler handler,
        Action<TEventHandler> subscribe,
        Action<TEventHandler> unsubscribe) where TEventHandler : Delegate
    {
        return new EventSubscription<TEventHandler>(handler, subscribe, unsubscribe);
    }

    public void Dispose()
    {
        _channel.Writer.Complete();
    }
}

public class EventSystem : IESystem, IEHandler<EventRequest>
{
    public ESystemPriority Priority
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    private Dictionary<string, EventRequest> _events = [];

    private readonly EventChannel<EventRequest> Channel = new();

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

    public void Update(World world, float dt)
    {
    }
}
