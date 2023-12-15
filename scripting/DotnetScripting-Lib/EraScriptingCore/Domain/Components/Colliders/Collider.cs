namespace EraScriptingCore.Domain.Components;

public enum ColliderType : uint
{
    None,
    Box,
    Capsule,
    Sphere,
    Mesh
}

public abstract class Collider : EComponent
{
    public ColliderType Type { get; protected set; }
}
