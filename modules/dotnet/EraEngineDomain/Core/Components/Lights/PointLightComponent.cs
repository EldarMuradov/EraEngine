using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Components.Lights;

public sealed class PointLightComponent : LightComponent
{
    internal override void InitializeComponentInternal(params object[] args)
    {
        LightData = (ILightData)args[0];
        LightType = LightType.Spot;
        PointLightData data = (PointLightData)LightData;
        initializePointLight(Entity.Id, data.Color.RGBAColor, data.Position, data.Attenuation, data.Range);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private extern static void initializePointLight(int id, Vector4 color, Vector3 position, Vector3 attenuation, float range);
}
