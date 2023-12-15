using System.Numerics;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Domain.Components.Lights;

public sealed class PointLightComponent : LightComponent
{
    public override void Initialize(params object[] args)
    {
        LightData = (ILightData)args[0];
        LightType = LightType.Spot;
        PointLightData data = (PointLightData)LightData;
        init(Entity.Id, data.Color.RGBAColor, data.Position, data.Attenuation, data.Range);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private extern static void init(int id, Vector4 color, Vector3 position, Vector3 attenuation, float range);
}
