using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading.Channels;

namespace EraEngine;

public class EventRequest
{
    public bool IsCompleted { get; set; } = false;

    public string Name { get; set; } = "Event";
}

public sealed class EventChannel<T> : IDisposable
{
    private readonly Channel<T> _channel = Channel.CreateUnbounded<T>();
    public event Action<T> Subscribers = null!;

    public Task Completion => _channel.Reader.Completion;

    public bool Post(T item)
    {
        Subscribers?.Invoke(item);
        return _channel.Writer.TryWrite(item);
    }

    public IAsyncEnumerable<T> ToAsyncEnumerable(CancellationToken token)
    {
        return _channel.Reader.ReadAllAsync(token);
    }

    internal class EventSubscription<TEventArg> : IDisposable
    {
        private readonly Action<TEventArg> _subscribe;
        private readonly Action _unsubscribe;

        private readonly EventChannel<TEventArg> _channel;

        public EventSubscription(
            EventChannel<TEventArg> channel,
            Action<TEventArg> subscribe,
            Action unsubscribe)
        {
            _subscribe = subscribe;
            _unsubscribe = unsubscribe;

            _channel = channel;
            _channel.Subscribers += _subscribe;
        }

        public void Dispose()
        {
            _channel.Subscribers -= _subscribe;
            _unsubscribe();
        }
    }

    public IDisposable Subscribe(Action<T> subscribe, Action unsubscribe)
    {
        return new EventSubscription<T>(this, subscribe, unsubscribe);
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

    public readonly EventChannel<EventRequest> Channel = new();

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

    public void Update(World world, float dt)
    {
    }
}
