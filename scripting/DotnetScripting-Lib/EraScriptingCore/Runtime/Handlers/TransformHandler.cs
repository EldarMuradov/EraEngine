using EraEngine.Components;
using EraEngine.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Core.Runtime;

public sealed class TransformHandler
{
    [UnmanagedCallersOnly(EntryPoint = "handle_transforms")]
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

    [UnmanagedCallersOnly(EntryPoint = "process_transform")]
    public static unsafe void ProcessTransform(IntPtr transformPtr, int id)
    {
        Matrix4x4 transforms = new();
        float[] mat = new float[16];
        float* ptr = (float*)transformPtr;
        int index = 0;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                transforms[i, j] = ptr[index];
                index++;
            }
        }
        Scene.Entities[id].GetComponent<TransformComponent>().SetTransformMatrix(transforms, false);

        Marshal.FreeHGlobal(transformPtr);
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