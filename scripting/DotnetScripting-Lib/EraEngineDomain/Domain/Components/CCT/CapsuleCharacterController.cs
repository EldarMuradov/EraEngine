using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Components;

public sealed class CapsuleCharacterController : CharacterController
{
    public float Radius { get; private set; }

    internal override void InitializeComponentInternal(params object[] args)
    {
        if (args.Length == 0)
            throw new ArgumentException("Runtime> You must put at least 1 argument!");
        Type = CharacterControllerType.Capsule;
        Radius = (float)args[0];
        initializeCapsuleCCT(Entity.Id, Radius);
    }

    public override void Move(Vector3 position)
    {
        // TODO: More complex logic
        base.Move(position);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void initializeCapsuleCCT(int id, float radius);
}
