using System.Runtime.InteropServices;

namespace EraEngine.Components;

public sealed class MeshCollider : Collider
{
    internal override void InitializeComponentInternal(params object[] args)
    {
        Type = ColliderType.Mesh;
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void initializeMeshColliderByReourceId(int id, ulong resourceId);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern void initializeMeshColliderByPath(int id, string path);
}
