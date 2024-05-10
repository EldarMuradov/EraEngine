using EraEngine.Components;
using EraEngine.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Core;

[StructLayout(LayoutKind.Sequential)]
public struct RaycastInfo
{
    public int Id;
    public float Distance;
    public uint HitCount;
    public Vector3 Position;
}

public static class Physics
{
    public static RaycastInfo Raycast(this RigidbodyComponent rb, Vector3 direction) 
    {
        // TODO: More checking barriers
        IntPtr vec = Memory.StructToIntPtr(direction);
        RaycastInfo info = raycast(rb.Entity.Id, vec);
        Memory.ReleaseIntPtr(vec);
        return info;
    }

    public static RaycastInfo Raycast(TransformComponent origin, Vector3 direction)
    {
        // TODO: More checking barriers
        IntPtr vec = Memory.StructToIntPtr(direction);
        RaycastInfo info = raycast(origin.Entity.Id, vec);
        Memory.ReleaseIntPtr(vec);
        return info;
    }

    #region P/I

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern RaycastInfo raycast(int id, IntPtr direction);

    #endregion
}
