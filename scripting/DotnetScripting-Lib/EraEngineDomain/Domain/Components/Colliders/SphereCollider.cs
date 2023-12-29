using System.Runtime.InteropServices;

namespace EraEngine.Components;

public sealed class SphereCollider : Collider
{
    internal override void InitializeComponentInternal(params object[] args)
    {
        if (args.Length == 0)
            throw new ArgumentException("Runtime> You must put at least 1 argument!");
        
        Radius = (float)args[0];
        Type = ColliderType.Sphere;
        initializeSphereCollider(Entity.Id, Radius);
    }

    public float Radius { get; private set; }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void initializeSphereCollider(int id, float radius);
}
