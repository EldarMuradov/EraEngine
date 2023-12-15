namespace EraScriptingCore.Domain.Components.Lights;

public enum LightType : uint 
{
    None,
    Directional,
    Point,
    Spot,
    Area
}

public abstract class LightComponent : EComponent
{
    public ILightData? LightData { get; protected set; }
    public LightType LightType { get; protected set; }
}
