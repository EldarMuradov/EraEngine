using System.Runtime.CompilerServices;

namespace EraEngine.Components;

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
    public ColliderType Type
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        protected set;
    }
}
