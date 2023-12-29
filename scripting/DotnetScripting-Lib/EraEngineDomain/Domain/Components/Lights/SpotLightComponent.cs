using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Components.Lights;

public sealed class SpotLightComponent : LightComponent
{
    internal override void InitializeComponentInternal(params object[] args)
    {
        LightData = (ILightData)args[0];
        LightType = LightType.Spot;
        SpotLightData data = (SpotLightData)LightData;
        initializeSpotLight(Entity.Id, data.Color.RGBAColor, data.Attenuation, data.Position, data.Direction, data.Range, data.InnerConeAngle, data.OuterConeAngle);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private extern static void initializeSpotLight(int id, Vector4 color, Vector3  attenuation, Vector3 position, Vector3 direction, float range, float inner, float outter);
}
