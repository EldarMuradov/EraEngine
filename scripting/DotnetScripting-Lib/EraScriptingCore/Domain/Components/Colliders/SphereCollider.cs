namespace EraScriptingCore.Domain.Components;

public sealed class SphereCollider : Collider
{
    public override void Initialize(params object[] args)
    {
        if (args.Length == 0)
            throw new ArgumentException("Runtime> You must put at least 1 argument!");
        
        Radius = (float)args[0];
        Type = ColliderType.Sphere;
    }

    public float Radius { get; private set; }
}
