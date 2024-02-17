using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace EraEngine.Components.Lights;

public interface ILightData
{
    Color Color 
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get; 
    }
}

public struct PointLightData : ILightData
{
    public Color Color 
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public Vector3 Position 
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public Vector3 Attenuation 
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public readonly float Range;

    public PointLightData(Color color, Vector3 pos, Vector3 att, float range)
    {
        Color = color;
        Position = pos;
        Attenuation = att;
        Range = range;
    }
}

public struct SpotLightData : ILightData
{
    public Color Color
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public Vector3 Direction
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public Vector3 Position
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public Vector3 Attenuation
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public readonly float Range;
    public readonly float InnerConeAngle;
    public readonly float OuterConeAngle;

    public SpotLightData(Color color, Vector3 dir, Vector3 pos, Vector3 att, float range, float inner, float outter)
    {
        Color = color;
        Direction = dir;
        Position = pos;
        Attenuation = att;
        Range = range;
        InnerConeAngle = inner;
        OuterConeAngle = outter;
    }
}

public struct DirLightData : ILightData
{
    public Color Color
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public Vector3 Direction
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public Vector3 Ambient
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
    }

    public DirLightData(Color color, Vector3 dir, Vector3 amb)
    {
        Color = color;
        Direction = dir;
        Ambient = amb;
    }
}

[StructLayout(LayoutKind.Sequential)]
public struct Color
{
    public Vector4 RGBAColor;
}
