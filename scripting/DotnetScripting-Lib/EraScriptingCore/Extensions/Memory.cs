using System.Numerics;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Extensions;

public static class Memory
{
    public static IntPtr Vector3ToIntPtr(Vector3 vector)
    {
        int size = Marshal.SizeOf(vector);
        IntPtr ptr = Marshal.AllocHGlobal(size);
        Marshal.StructureToPtr(vector, ptr, false);
        return ptr;
    }

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
