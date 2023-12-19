namespace EraScriptingCore.Domain.Components;

public sealed class MeshCollider : Collider
{
    public override void Initialize(params object[] args)
    {
        Type = ColliderType.Mesh;
    }
}
