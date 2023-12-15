using System.Numerics;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Domain.Components;

public sealed class TransformComponent : EComponent
{
    private Vector3 _position;
    private Quaternion _rotation;
    private Vector3 _scale;

    public void SetPosition(Vector3 position, bool sync = true)
    {
        _position = position;
        if(sync)
            setPosition(Entity.Id, _position);
    }

    public void SetRotation(Quaternion rotation, bool sync = true)
    {
        _rotation = rotation;
        if (sync)
            setRotation(Entity.Id, _rotation);
    }

    public void SetScale(Vector3 scale, bool sync = true)
    {
        _scale = scale;
        if (sync)
            setScale(Entity.Id, _scale);
    }

    public void SetTransformMatrix(Matrix4x4 transform, bool sync = true)
    {
        Matrix4x4.Decompose(transform, out var pos, out var rot, out var scale);

        SetPosition(pos, sync);
        SetRotation(rot, sync);
        SetScale(scale, sync);
        
        if (sync)
            setTransformMatrix(Entity.Id, transform);
    }

    #region P/I

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setPosition(int id, Vector3 position);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setRotation(int id, Quaternion rotation);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setScale(int id, Vector3 scale);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setTransformMatrix(int id, Matrix4x4 transform);

    #endregion
}
