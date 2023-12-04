namespace EraScriptingCore.Domain.Components;

public enum CharacterControllerType 
{
    None,
    Box,
    Capsule
}

public abstract class CharacterController(CharacterControllerType type) : EComponent
{
    public CharacterControllerType Type { get; } = type;
}
