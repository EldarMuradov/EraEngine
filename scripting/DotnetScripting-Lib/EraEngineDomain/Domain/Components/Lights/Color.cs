using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Components.Lights;

public interface ILightData
{
    Color Color { get; }
}

public struct PointLightData : ILightData
{
    public Color Color { get; }

    public readonly float Range;

    public Vector3 Position { get; }
    public Vector3 Attenuation { get; }

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
    public Color Color { get; }

    public Vector3 Direction { get; }
    public Vector3 Position { get; }
    public Vector3 Attenuation { get; }

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
    public Color Color { get; }
    public Vector3 Direction { get; }
    public Vector3 Ambient { get; }

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
