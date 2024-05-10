using EraEngine.Components;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Core;

public class TransformHandler : IEHandler
{
    [UnmanagedCaller]
    public static unsafe void ProcessTransform(IntPtr transformPtr, int id)
    {
        Matrix4x4 transforms = new();

        float[] mat = new float[16];

        float* ptr = (float*)transformPtr;

        int index = 0;

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                transforms[i, j] = ptr[index];
                index++;
            }
        }

        EWorld.SceneWorld.GetEntity(id).GetComponent<TransformComponent>().SetTransformMatrix(transforms, false);

        Marshal.FreeHGlobal(transformPtr);
    }
}