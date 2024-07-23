namespace EraEngine.Components;

public sealed class EMesh : EResource
{
    public string Path { get; }

    public EMesh(ulong resourceId, string path,string name = "DefaultMesh")
    {
        Id = resourceId;
        Path = path;
        Name = name;
    }
}

public sealed class MeshRendererComponent : EComponent, IRenderable
{
    public EMesh Mesh { get; private set; }

    public void Render(float dt) { }

    internal override void InitializeComponentInternal(params object[] args)
    {
        
    }
}
