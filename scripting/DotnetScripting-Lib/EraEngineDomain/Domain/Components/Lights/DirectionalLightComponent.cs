using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Components.Lights;

public sealed class DirectionalLightComponent : LightComponent
{
    internal override void InitializeComponentInternal(params object[] args)
    {
        LightData = (ILightData)args[0];
        LightType = LightType.Spot;
        DirLightData data = (DirLightData)LightData;
        initializeDirLight(Entity.Id, data.Color.RGBAColor, data.Direction, data.Ambient);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private extern static void initializeDirLight(int id, Vector4 color, Vector3 direction, Vector3 ambient);
}
