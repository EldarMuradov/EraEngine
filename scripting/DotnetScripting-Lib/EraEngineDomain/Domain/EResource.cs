using EraEngine.Components;
using System.Runtime.InteropServices;

namespace EraEngine;

public abstract class EResource
{
    public ulong Id { get; protected set; }

    public string Name { get; protected set; } = "";
}

public static class ResourceExtensions
{
    public static Result<EResource> LoadMeshFromFile(string path, string name = "DefaultMesh")
    {
        var meshId = loadMeshResourceAsyncByPath(path);
        return Result<EResource>.Success(new EMesh(meshId, path, name));
    }

    public static Result<EResource> LoadMeshById(ulong id, string name = "DefaultMesh")
    {
        if(loadMeshResourceAsyncById(id))
            return Result<EResource>.Success(new EMesh(id, name));
        return Result<EResource>.Failure($"Resource> Failed to load mesh with {id} id");
    }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern bool loadMeshResourceAsyncById(ulong id);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern ulong loadMeshResourceAsyncByPath(string path);
}