using System.Runtime.InteropServices;

namespace EraEngine.Extensions;

public static class Memory
{
    public static IntPtr StructToIntPtr<T>(T @struct) where T : struct
    {
        int size = Marshal.SizeOf(@struct);
        IntPtr ptr = Marshal.AllocHGlobal(size);
        Marshal.StructureToPtr(@struct, ptr, false);
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

    public static unsafe int GetStringLength(IntPtr str)
    {
        var ptr = (byte*)str;
        var length = 0;
        while (*(ptr + length) != 0)
            length++;
        return length;
    }

    public static void ReleaseIntPtr(IntPtr data)
    {
        Marshal.FreeHGlobal(data);
    }
}
