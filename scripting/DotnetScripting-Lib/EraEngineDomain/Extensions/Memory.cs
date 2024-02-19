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

    public unsafe static T[] Create<T>(void* source, int length)
    {
        var type = typeof(T);
        var sizeInBytes = Marshal.SizeOf(typeof(T));
        T[] output = new T[length];

        if (type.IsPrimitive)
        {
            var handle = GCHandle.Alloc(output, GCHandleType.Pinned);
            var destination = (byte*)handle.AddrOfPinnedObject().ToPointer();
            var byteLength = length * sizeInBytes;

            for (int i = 0; i < byteLength; i++)
                destination[i] = ((byte*)source)[i];
            handle.Free();
        }
        else if (type.IsValueType)
        {
            if (!type.IsLayoutSequential && !type.IsExplicitLayout)
            {
                throw new InvalidOperationException(string.Format("{0} does not define a StructLayout attribute", type));
            }

            IntPtr sourcePtr = new IntPtr(source);
            for (int i = 0; i < length; i++)
            {
                IntPtr p = new IntPtr((byte*)source + i * sizeInBytes);
                output[i] = (T)System.Runtime.InteropServices.Marshal.PtrToStructure(p, typeof(T));
            }
        }
        else
        {
            throw new InvalidOperationException(string.Format("{0} is not supported", type));
        }

        return output;
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
