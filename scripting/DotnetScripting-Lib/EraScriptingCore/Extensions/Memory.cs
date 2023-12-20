using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Extensions;

public static class Memory
{
    public static IntPtr StructToIntPtr<T>(T @struct) where T : struct
    {
        int size = Marshal.SizeOf(@struct);
        IntPtr ptr = Marshal.AllocHGlobal(size);
        Marshal.StructureToPtr(@struct, ptr, false);
        return ptr;
    }

    //public static unsafe Vector3 PtrToVector3(IntPtr ptr)
    //{
    //    int* v = (int*)ptr.ToPointer();
    //    Vector3 vector = new Vector3((float)*v, (float)*(++v), (float)*(++v));
    //    return vector;
    //}

    public static unsafe void CopyToManaged<T>(IntPtr source, T[] destination, int startIndex, int length)
    {
        ArgumentNullException.ThrowIfNull(destination);

        ArgumentNullException.ThrowIfNull(source);
        ArgumentOutOfRangeException.ThrowIfNegative(startIndex);
        ArgumentOutOfRangeException.ThrowIfNegative(length);

        new Span<T>((void*)source, length).CopyTo(new Span<T>(destination, startIndex, length));
    }

    public static void ReleaseIntPtr(IntPtr data)
    {
        Marshal.FreeHGlobal(data);
    }
}
