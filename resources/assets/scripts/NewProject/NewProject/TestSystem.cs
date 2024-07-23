using EraEngine;

namespace NewProject;

public class TestSystem : IESystem
{
    public ESystemPriority Priority { get; }

    public TestSystem()
    {
        Priority = ESystemPriority.Normal;
    }

    public void Update(World world, float dt)
    {
        //Debug.Log("Log from test system");
    }
}
