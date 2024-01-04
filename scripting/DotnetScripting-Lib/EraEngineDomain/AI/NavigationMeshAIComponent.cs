using EraEngine.Components;
using System.Numerics;

namespace EraEngine.AI;

public enum NavigationType 
{
    None, 
    A_Star,
    Dijkstra
}

public sealed class NavigationMeshAIComponent : EComponent
{
    public NavigationType Type { get; private set; }

    private Vector3 _destination;
    private Vector3 _prevDestination;
    
    internal override void InitializeComponentInternal(params object[] args) 
    {
        if (args.Length != 1)
            throw new ArgumentException("Wrong number of arguments!");
        Type = (NavigationType)args[0];
    }

    public void SetDestination(Vector3 dest)
    {
        _prevDestination = _destination;
        _destination = dest;
    }
}
