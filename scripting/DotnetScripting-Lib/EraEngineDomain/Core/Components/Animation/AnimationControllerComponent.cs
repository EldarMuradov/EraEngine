using EraEngine.Components;
using System.Runtime.InteropServices;

namespace EraEngine;

public sealed class Animation
{
    private readonly ulong _resourceId;

    public string Name { get; set; }

    public Animation(ulong resourceId, string name = "DefaultAnimation")
    {
        _resourceId = resourceId;

        Name = name;
    }
}

[RequireComponent<MeshRendererComponent>]
public class AnimationControllerComponent : EComponent
{
    private List<Animation> _animations = [];

    public Animation CurrentAnimation { get; private set; } = default!;

    public void SetAnimationByIndex(int index)
    {
        if (setAnimationByIndex(Entity.Id, index))
            CurrentAnimation = _animations[index];
        else
            Debug.LogError($"Animation> Failed to set animation with {index} index");
    }

    private void LoadAnimations()
    {
        loadAnimationsFromMeshSkeleton(Entity.Id);
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern bool setAnimationByIndex(int id, int state);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern string loadAnimationsFromMeshSkeleton(int id);
}
