using EraEngine.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Components;

public sealed class TransformComponent : EComponent
{
    private Vector3 _position;
    private Quaternion _rotation;
    private Vector3 _scale;

    public Vector3 LocalPosition { get; set; }
    public Quaternion LocalRotation { get; set; }

    public Vector3 GetPosition() => _position;

    public Quaternion GetRotation() => _rotation;

    public Vector3 GetScale() => _scale;

    public void SetPosition(Vector3 position, bool sync = true)
    {
        _position = position;

        LocalPosition = Entity.Parent is null 
            ? LocalPosition = position
            : Entity.Parent.GetComponent<TransformComponent>().GetPosition() - _position;

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
        LocalRotation = rotation;

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
        Matrix4x4.Decompose(transform, out var scale, out var rot, out var pos);

        SetPosition(pos, sync);
        SetRotation(rot, sync);
        SetScale(scale, sync);

        if (sync)
        {
            IntPtr trs = Memory.StructToIntPtr(transform);
            setTransformMatrix(Entity.Id, trs);
            Memory.ReleaseIntPtr(trs);
        }
    }

    public void SetParent(TransformComponent transform)
    {
        // TODO
    }

    public static Vector3 ToEulerAngles(Quaternion q)
    {
        Vector3 angles = new();

        // roll / x
        double sinr_cosp = 2 * (q.W * q.X + q.Y * q.Z);
        double cosr_cosp = 1 - 2 * (q.X * q.X + q.Y * q.Y);
        angles.X = (float)Math.Atan2(sinr_cosp, cosr_cosp);

        // pitch / y
        double sinp = 2 * (q.W * q.Y - q.Z * q.X);
        if (Math.Abs(sinp) >= 1)
        {
            angles.Y = (float)Math.CopySign(Math.PI / 2, sinp);
        }
        else
        {
            angles.Y = (float)Math.Asin(sinp);
        }

        // yaw / z
        double siny_cosp = 2 * (q.W * q.Z + q.X * q.Y);
        double cosy_cosp = 1 - 2 * (q.Y * q.Y + q.Z * q.Z);
        angles.Z = (float)Math.Atan2(siny_cosp, cosy_cosp);

        return angles;
    }

    public Vector3 InverseTransformPoint(Vector3 point)
    {
        Vector3 localPoint = point - _position;

        var eulerAngles = ToEulerAngles(_rotation);

        float radX = (float)(-eulerAngles.X * (Math.PI / 180f));
        float radY = (float)(-eulerAngles.Y * (Math.PI / 180f));
        float radZ = (float)(-eulerAngles.Z * (Math.PI / 180f));

        localPoint = new Vector3(
        (float)(localPoint.X * Math.Cos(radY) * Math.Cos(radZ) + localPoint.Y * (Math.Cos(radZ) * Math.Sin(radX) * Math.Sin(radY) - Math.Cos(radX) * Math.Sin(radZ)) + localPoint.Z * (Math.Cos(radX) * Math.Cos(radZ) * Math.Sin(radY) + Math.Sin(radX) * Math.Sin(radZ))),
        (float)(localPoint.X * Math.Cos(radY) * Math.Sin(radZ) + localPoint.Y * (Math.Cos(radX) * Math.Cos(radZ) + Math.Sin(radX) * Math.Sin(radY) * Math.Sin(radZ)) + localPoint.Z * (-Math.Cos(radZ) * Math.Sin(radX) + Math.Cos(radX) * Math.Sin(radY) * Math.Sin(radZ))),
        (float)(localPoint.X * -Math.Sin(radY) + localPoint.Y * Math.Cos(radY) * Math.Sin(radX) + localPoint.Z * Math.Cos(radX) * Math.Cos(radY))
        );

        localPoint = new Vector3(localPoint.X / _scale.X, localPoint.Y / _scale.Y, localPoint.Z / _scale.Z);
        return localPoint;
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
