namespace EraEngine;

public class BackgroundServiceSystem : IESystem
{
    public ESystemPriority Priority { get; }

    public BackgroundServiceSystem()
    {
        Priority = ESystemPriority.Low;
    }

    public void Update(float dt)
    {
    }
}