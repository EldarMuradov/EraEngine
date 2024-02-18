using System.Runtime.CompilerServices;

namespace EraEngine;

public interface ICollectionSystem : IESystem
{
    int FilterId
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }
}
