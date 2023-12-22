namespace EraScriptingCore.Domain.Components;

public sealed class MeshCollider : Collider
{
    internal override void InitializeComponentInternal(params object[] args)
    {
        Type = ColliderType.Mesh;
    }
}
