using EraEngine.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Components;

public sealed class TransformComponent : EComponent
{
    private Vector3 _position;
    private Quaternion _rotation;
    private Vector3 _scale;

    public void SetPosition(Vector3 position, bool sync = true)
    {
        _position = position;
        if (sync)
        {
            IntPtr vec = Memory.StructToIntPtr(position);
            setPosition(Entity.Id, vec);
            Memory.ReleaseIntPtr(vec);
        }
    }

    public void SetRotation(Quaternion rotation, bool sync = true)
    {
        _rotation = rotation;
        if (sync)
        {
            IntPtr q = Memory.StructToIntPtr(rotation);
            setRotation(Entity.Id, q);
            Memory.ReleaseIntPtr(q);
        }
    }

    public void SetScale(Vector3 scale, bool sync = true)
    {
        _scale = scale;
        if (sync)
        {
            IntPtr vec = Memory.StructToIntPtr(scale);
            setScale(Entity.Id, vec);
            Memory.ReleaseIntPtr(vec);
        }
    }

    public void SetTransformMatrix(Matrix4x4 transform, bool sync = true)
    {
        IntPtr trs = Memory.StructToIntPtr(transform);
        Matrix4x4.Decompose(transform, out var pos, out var rot, out var scale);

        SetPosition(pos, sync);
        SetRotation(rot, sync);
        SetScale(scale, sync);
        
        if (sync)
            setTransformMatrix(Entity.Id, trs);

        Memory.ReleaseIntPtr(trs);
    }

    internal override void InitializeComponentInternal(params object[] args) { }

    #region P/I

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setPosition(int id, IntPtr position);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setRotation(int id, IntPtr rotation);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setScale(int id, IntPtr scale);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setTransformMatrix(int id, IntPtr transform);

    #endregion
}
