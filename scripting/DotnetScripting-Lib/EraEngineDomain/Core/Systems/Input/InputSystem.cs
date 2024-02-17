using System.Runtime.CompilerServices;

namespace EraEngine;

public class InputSystem : IESystem
{
    public ESystemPriority Priority
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public InputSystem()
    {
        Priority = ESystemPriority.High;
    }

    public void Update(float dt)
    {

    }
}
