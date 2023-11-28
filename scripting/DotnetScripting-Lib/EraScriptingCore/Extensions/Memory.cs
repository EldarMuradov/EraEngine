using System.Numerics;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Extensions;

public class Memory
{
    public static IntPtr Vector3ToIntPtr(Vector3 vector)
    {
        int size = Marshal.SizeOf(vector);
        IntPtr ptr = Marshal.AllocHGlobal(size);
        Marshal.StructureToPtr(vector, ptr, false);
        return ptr;
    }

    public static void ReleaseIntPtr(IntPtr data)
    {
        Marshal.FreeHGlobal(data);
    }
}
