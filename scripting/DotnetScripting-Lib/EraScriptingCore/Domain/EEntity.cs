using EraScriptingCore.Domain.Components;

namespace EraScriptingCore.Domain;

public class EEntity
{
    public uint Id { get; init; }

    public string Name { get; init; }

    public List<EComponent> Components = new();

    public EEntity(uint id, string name)
    {
        (Id, Name) = (id, name);
    }
}
