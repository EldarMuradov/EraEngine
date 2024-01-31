namespace EraEngine;

public enum ESystemPriority
{
    None = 0,
    High,
    Normal,
    Low
}

public class ESystemManager : IEManager
{
    private static PriorityQueue<IESystem, ESystemPriority> _prioritySystemsQueue = new();

    private static Dictionary<string, IESystem> _systems = [];

    public static T RegisterSystem<T>() where T : IESystem, new()
    {
        T system = new();
        _prioritySystemsQueue.Enqueue(system, system.Priority);
        _systems.Add(system.GetType().Name, system);
        return system;
    }

    public static IESystem RegisterSystem(IESystem system)
    {
        _prioritySystemsQueue.Enqueue(system, system.Priority);
        _systems.Add(system.GetType().Name, system);
        return system;
    }

    public static void ReleaseSystems()
    {
        _prioritySystemsQueue.Clear();
        _systems.Clear();
    }

    public static PriorityQueue<IESystem, ESystemPriority> GetSystemsQueue()
    {
        PriorityQueue<IESystem, ESystemPriority> priorityQueue = new(_prioritySystemsQueue.UnorderedItems);
        return priorityQueue;
    }

    public static T GetSystem<T>() where T : IESystem, new()
    {
        if (_systems.TryGetValue(typeof(T).Name, out var system))
            return (T)system;
        return default!;
    }
}
