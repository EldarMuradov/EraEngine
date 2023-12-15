using EraScriptingCore.Domain;
using EraScriptingCore.Domain.Components;
using EraScriptingCore.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Core.Runtime;

public sealed class TransformHandler
{
    [UnmanagedCallersOnly(EntryPoint = "handle_transform")]
    public static unsafe void HandleTransforms(IntPtr transformPtr, IntPtr idsPtr, int size)
    {
        int[] ids = new int[size];
        Marshal.Copy(idsPtr, ids, 0, size);

        Matrix4x4[] transforms = new Matrix4x4[size];
        Memory.CopyToManaged(transformPtr, transforms, 0, size * sizeof(Matrix4x4));

        for (int i = 0; i < size; i++)
        {
            Scene.Entities[ids[i]].GetComponent<TransformComponent>().SetTransformMatrix(transforms[i], false);
        }

        Marshal.FreeHGlobal(transformPtr);
        Marshal.FreeHGlobal(idsPtr);
    }

    [UnmanagedCallersOnly(EntryPoint = "handle_positions")]
    public static unsafe void HandlePosition(IntPtr positionsPtr, IntPtr idsPtr, int size)
    {
        int[] ids = new int[size];
        Marshal.Copy(idsPtr, ids, 0, size);

        Vector3[] positions = new Vector3[size];
        Memory.CopyToManaged(positionsPtr, positions, 0, size * sizeof(Vector3));

        for (int i = 0; i < size; i++)
        {
            Scene.Entities[ids[i]].GetComponent<TransformComponent>().SetPosition(positions[i], false);
        }

        Marshal.FreeHGlobal(positionsPtr);
        Marshal.FreeHGlobal(idsPtr);
    }

    [UnmanagedCallersOnly(EntryPoint = "handle_rotations")]
    public static unsafe void HandleRotations(IntPtr rotationsPtr, IntPtr idsPtr, int size)
    {
        int[] ids = new int[size];
        Marshal.Copy(idsPtr, ids, 0, size);

        Quaternion[] rotations = new Quaternion[size];
        Memory.CopyToManaged(rotationsPtr, rotations, 0, size * sizeof(Quaternion));

        for (int i = 0; i < size; i++)
        {
            Scene.Entities[ids[i]].GetComponent<TransformComponent>().SetRotation(rotations[i], false);
        }

        Marshal.FreeHGlobal(rotationsPtr);
        Marshal.FreeHGlobal(idsPtr);
    }

    [UnmanagedCallersOnly(EntryPoint = "handle_scales")]
    public static unsafe void HandleScales(IntPtr scalesPtr, IntPtr idsPtr, int size)
    {
        int[] ids = new int[size];
        Marshal.Copy(idsPtr, ids, 0, size);

        Vector3[] scales = new Vector3[size];
        Memory.CopyToManaged(scalesPtr, scales, 0, size * sizeof(Vector3));

        for (int i = 0; i < size; i++)
        {
            Scene.Entities[ids[i]].GetComponent<TransformComponent>().SetScale(scales[i], false);
        }

        Marshal.FreeHGlobal(scalesPtr);
        Marshal.FreeHGlobal(idsPtr);
    }
}