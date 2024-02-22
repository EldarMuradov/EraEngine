using System.Runtime.CompilerServices;

namespace EraEngine;

public interface IESystem
{
    ESystemPriority Priority
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    void Update(World world, float dt);
}
