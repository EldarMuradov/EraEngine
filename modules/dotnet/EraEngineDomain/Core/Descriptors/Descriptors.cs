using EraEngine.Components;
using System.Runtime.InteropServices;

namespace EraEngine.Descriptors;

[StructLayout(LayoutKind.Sequential)]
public struct RigidbodyDescriptor
{
    public RigidbodyType Type;
}