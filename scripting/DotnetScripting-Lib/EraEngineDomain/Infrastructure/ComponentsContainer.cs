using EraEngine.Components;
namespace EraEngine.Infrastructure;

public sealed class ComponentsContainer
{

    public Dictionary<string, EComponent> Components = [];

    private Dictionary<string, EComponent> _components = [];

    public List<string> TempCompData = [];

    public Dictionary<string, EComponent>.ValueCollection Values 
    { 
        get => Components.Values;
    }

    public Dictionary<string, EComponent>.ValueCollection ActiveValues
    {
        get => _components.Values;
    }

    internal bool TryGetValue(string key, out EComponent component)
    {
        return _components.TryGetValue(key, out component);
    }

    internal void Add(string name, EComponent component)
    {
        _components.Add(name, component);
    }

    internal bool ContainsKey(string key)
    { 
        return _components != null && _components.ContainsKey(key);
    }

    internal void Remove(string name)
    {
        Components.Remove(name);
        _components.Remove(name);
    }

    internal void SyncComponents()
    {
        Components = new(_components);
    }
}
