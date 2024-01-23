namespace EraEngine;

public class InputSystem : IESystem
{
    public ESystemPriority Priority { get; }

    public InputSystem()
    {
        Priority = ESystemPriority.High;
    }

    public void Update(float dt)
    {

    }
}
