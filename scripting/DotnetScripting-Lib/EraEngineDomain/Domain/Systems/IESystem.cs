namespace EraEngine;

public interface IESystem
{
    ESystemPriority Priority { get; }

    void Update(float dt);
}
