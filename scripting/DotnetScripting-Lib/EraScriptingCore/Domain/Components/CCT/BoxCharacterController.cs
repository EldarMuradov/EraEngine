using System.Numerics;

namespace EraScriptingCore.Domain.Components;

public sealed class BoxCharacterController : CharacterController
{
    public override void Initialize(params object[] args)
    {
        if (args.Length < 3)
            throw new ArgumentException("Runtime> You must put at least 3 arguments!");

        Height = (float)args[0];
        Length = (float)args[1];
        Width = (float)args[2];
        Type = CharacterControllerType.Box;
    }

    /// <summary>
    /// X
    /// </summary>
    public float Height { get; private set; }

    /// <summary>
    /// Y
    /// </summary>
    public float Length { get; private set; }

    /// <summary>
    /// Z
    /// </summary>
    public float Width { get; private set; }

    public override void Move(Vector3 position)
    {
        // TODO: More complex logic
        base.Move(position);
    }
}