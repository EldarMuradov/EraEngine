namespace EraScriptingCore.Domain.Components;

public enum ColliderType : uint
{
    None,
    Box,
    Capsule,
    Sphere,
    Mesh
}

public abstract class Collider(ColliderType type) : EComponent
{
    public ColliderType Type { get; } = type;
}
