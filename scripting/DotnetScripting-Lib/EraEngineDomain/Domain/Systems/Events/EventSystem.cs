namespace EraEngine;

public class EventSystem : IESystem
{
    public ESystemPriority Priority { get; }

    public EventSystem()
    {
        Priority = ESystemPriority.Normal;
    }

    public void Update(float dt)
    {
    }
}
