using EraEngine.Components;
using EraEngine.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.AI;

public enum NavigationType : byte
{
    None,
    A_Star,
    Dijkstra
}

public sealed class NavigationComponent : EComponent
{
    public NavigationType Type { get; private set; }

    private Vector3 _destination;

    internal override void InitializeComponentInternal(params object[] args) 
    {
        if (args.Length != 1)
            throw new ArgumentException("Wrong number of arguments!");
        Type = (NavigationType)args[0];

        initializeNavigationComponent(Entity.Id, (byte)Type);
    }

    public unsafe void SetDestination(Vector3 dest)
    {
        _destination = dest;
        IntPtr destPtr = Memory.StructToIntPtr(_destination);
        setDestination(Entity.Id, destPtr);
        Memory.ReleaseIntPtr(destPtr);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setDestination(int id, IntPtr destPtr);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void initializeNavigationComponent(int id, byte type);
}
