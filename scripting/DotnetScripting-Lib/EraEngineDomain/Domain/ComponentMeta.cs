using System.Runtime.CompilerServices;

namespace EraEngine;

static class ComponentIdCounter
{
    internal static int Counter = -1;
}

public static class ComponentMeta<T>
{
    public static int Id
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private set;
    }

    static ComponentMeta()
    {
        Id = Interlocked.Increment(ref ComponentIdCounter.Counter);
    }
}