namespace EraEngine.Components;

public sealed class MeshRendererComponent : EComponent, IRenderable
{
    public void Render(float dt) { }

    internal override void InitializeComponentInternal(params object[] args) { }
}
