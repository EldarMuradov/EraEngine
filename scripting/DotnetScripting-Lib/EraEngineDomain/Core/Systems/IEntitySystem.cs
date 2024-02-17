using System.Runtime.CompilerServices;

namespace EraEngine;

public interface IEntitySystem : IESystem
{
    List<EEntity> Entities
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }
}
