using EraEngine.Components;
using System.Runtime.InteropServices;

namespace EraEngine;

public sealed class EAnimation : EResource
{
    public EAnimation(ulong resourceId, string name = "DefaultAnimation")
    {
        Id = resourceId;

        Name = name;
    }
}

[RequireComponent<MeshRendererComponent>]
public class AnimationControllerComponent : EComponent
{
    private List<EAnimation> _animations = [];

    public EAnimation CurrentAnimation { get; private set; } = default!;

    public void SetAnimationByIndex(int index)
    {
        if (setAnimationByIndex(Entity.Id, index))
            CurrentAnimation = _animations[index];
        else
            Debug.LogError($"Animation> Failed to set animation with {index} index");
    }

    public void SetAnimation(in EAnimation animation)
    {
        if (!setAnimationByResourceHandle(Entity.Id, animation.Id))
            Debug.LogError($"Animation> Failed to set animation with {animation.Id} resource handle");
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
    private static extern bool setAnimationByIndex(int id, int index);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern bool setAnimationByResourceHandle(int id, ulong handle);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern string loadAnimationsFromMeshSkeleton(int id);
}
