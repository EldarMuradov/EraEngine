using System.Numerics;

namespace EraScriptingCore.Domain.Components;

public class TransformComponent : EComponent
{
    public Vector3 Position;
    public Quaternion Rotation;
    public Vector3 Scale;
}
