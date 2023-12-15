using EraScriptingCore.Domain.Components;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Domain.Descriptors;

[StructLayout(LayoutKind.Sequential)]
public struct RigidbodyDescriptor
{
    public RigidbodyType Type;
}