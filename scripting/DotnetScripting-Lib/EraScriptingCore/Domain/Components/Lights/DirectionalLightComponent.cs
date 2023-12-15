using System.Numerics;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Domain.Components.Lights;

public sealed class DirectionalLightComponent : LightComponent
{
    public override void Initialize(params object[] args)
    {
        LightData = (ILightData)args[0];
        LightType = LightType.Spot;
        DirLightData data = (DirLightData)LightData;
        init(Entity.Id, data.Color.RGBAColor, data.Direction, data.Ambient);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private extern static void init(int id, Vector4 color, Vector3 direction, Vector3 ambient);
}
