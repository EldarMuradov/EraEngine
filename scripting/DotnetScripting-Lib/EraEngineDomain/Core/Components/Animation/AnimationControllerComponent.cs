using EraEngine.Components;
using System.Runtime.InteropServices;

namespace EraEngine;

public sealed class Animation
{
    public readonly ulong ResourceId;

    public string Name { get; set; }

    public Animation(ulong resourceId, string name = "DefaultAnimation")
    {
        ResourceId = resourceId;

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

    public void SetAnimation(in Animation animation)
    {
        if (!setAnimationByResourceHandle(Entity.Id, animation.ResourceId))
            Debug.LogError($"Animation> Failed to set animation with {animation.ResourceId} resource handle");
    }

    internal override void InitializeComponentInternal(params object[] args)
    {
        base.InitializeComponentInternal(args);

        LoadAnimations();
    }

    private void LoadAnimations()
    {
        loadAnimationsFromMeshSkeleton(Entity.Id); // TODO
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern bool setAnimationByIndex(int id, int state);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern bool setAnimationByResourceHandle(int id, ulong handle);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern string loadAnimationsFromMeshSkeleton(int id);
}
