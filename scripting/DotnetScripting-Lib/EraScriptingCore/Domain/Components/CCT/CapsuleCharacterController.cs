using System.Numerics;

namespace EraScriptingCore.Domain.Components;

public sealed class CapsuleCharacterController : CharacterController
{
    public override void Initialize(params object[] args)
    {
        if (args.Length == 0)
            throw new ArgumentException("Runtime> You must put at least 1 argument!");
        Type = CharacterControllerType.Capsule;
    }

    public float Radius { get; private set; }

    public override void Move(Vector3 position)
    {
        // TODO: More complex logic
        base.Move(position);
    }
}
