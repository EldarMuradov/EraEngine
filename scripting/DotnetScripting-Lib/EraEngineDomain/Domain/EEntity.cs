using EraEngine.Components;
using EraEngine.Core;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine;

public class EEntity
{
    public unsafe EEntity(int id, string name)
    {
        (Id, Name) = (id, name);
        CreateComponentInternal<TransformComponent>();

        Scene.Add(this);
    }

    public EEntity()
    {
    }

    public int Id { get; init; }

    public string Name { get; init; } = null!;

    public Dictionary<string, EComponent> Components = new();

    public EEntity? Parent;

    public List<EEntity> Childs = new();

    public bool IsInitialized { get; private set; } = false;

    public bool ActiveSelf
    {
        get
        {
            return _activeSelf;
        }
        set
        {
            _activeSelf = value;
            //setActive(Id, _activeSelf);
        }
    }

    private bool _activeSelf = true;

    public Dictionary<string, EComponent> _components = new();

    #region Pipeline Methods

    public void Start()
    {
        IsInitialized = true;
        SyncComponents();

        var components = Components.Values;
        foreach (var comp in components)
            comp.Start();
    }

    public void Update(float dt)
    {
        var components = Components.Values;
        foreach (var comp in components)
            comp.Update(dt);
        SyncComponents();
    }

    private void SyncComponents()
    {
        Components = _components;
    }

    #endregion

    #region  Core Logic

    public T? GetComponent<T>() where T : EComponent, new()
    {
        if (_components.TryGetValue(typeof(T).Name, out var comp))
            return (T)comp;
        return null;
    }

    public T CreateComponent<T>(params object[] args) where T : EComponent, new()
    {
        var comp = GetComponent<T>();

        if (comp != null)
            return comp;

        comp = new T();

        if (comp is null)
        {
            Debug.LogError($"Scripting> Failed to create {nameof(T)} component!");
            return null!;
        }

        if (comp is Script)
            createScript(Id, comp.GetType().Name);

        comp.Entity = this;
        comp.InitializeComponentInternal(args);

        _components.Add(comp.GetType().Name, comp);

        if(IsInitialized)
            comp.Start();

        return comp;
    }

    public void RemoveComponent<T>(bool sync = false) where T : EComponent, new()
    {
        var comp = GetComponent<T>() ??
            throw new NullReferenceException("Runtime> Failed to remove component! Value is null.");

        var compname = comp.GetType().Name;

        _components.Remove(compname);
        if (sync)
            removeComponent(Id, compname);
    }

    public static EEntity Instantiate(EEntity original, Vector3 position, Quaternion rotation, EEntity? parent = null)
    {
        int newId = Scene.Entities.Count + 100000;

        if (parent != null)
            instantiate(original.Id, newId, parent.Id);
        else
            instantiate(original.Id, newId, -1);

        EEntity instance = new(newId, original.Name + newId.ToString());

        if (parent != null)
            instance.Parent = parent;

        foreach (var comp in original._components)
            instance.CopyComponent(comp.Value);

        var transform = instance.GetComponent<TransformComponent>()!;
        transform.SetPosition(position);
        transform.SetRotation(rotation);

        Scene.Entities.Add(newId, instance);

        return instance;
    }

    public static EEntity Instantiate(EEntity original, EEntity? parent = null)
    {
        int newId = Scene.Entities.Count + 100000;

        if (parent != null)
            instantiate(original.Id, newId, (int)parent.Id);
        else
            instantiate(original.Id, newId, -1);

        EEntity instance = new(newId, original.Name + newId.ToString());

        if (parent != null)
            instance.Parent = parent;

        foreach (var comp in original._components)
            instance.CopyComponent(comp.Value);

        Scene.Entities.Add(newId, instance);

        return instance;
    }

    public void AddChild(EEntity entity)
    {
        Childs.Add(entity);
    }

    public EComponent CopyComponent(EComponent component)
    {
        var comp = component;

        _components.Add(comp.GetType().Name, comp);

        return comp;
    }

    public void RemoveChild(EEntity entity)
    {
        Childs.Remove(entity);
    }

    public void Release()
    {
        release(Id);
        Scene.Entities.Remove(Id);
    } 

    #endregion

    #region Internal

    public T CreateComponentInternal<T>(params object[] args) where T : EComponent, new()
    {
        var comp = GetComponent<T>();

        if (comp != null)
            return comp;

        comp = new T();

        if (comp is null)
        {
            Debug.LogError($"Scripting> Failed to create {nameof(T)} component!");
            return null!;
        }

        comp.Entity = this;
        comp.InitializeComponentInternal(args);

        _components.Add(comp.GetType().Name, comp);

        if (IsInitialized)
            comp.Start();

        return comp;
    }

    internal void RemoveComponent(string name, bool sync = true)
    {
        _components.Remove(name);
        if (sync)
            removeComponent(Id, name);
    }

    internal void AddComponentFromInstance(EComponent comp, string name, bool from = true)
    {
        if (comp != null)
        {
            _components.Add(name, comp);
            comp.Entity = this;
            comp.Start();
            if (from)
                createScript(Id, name);
        }
    }

    #endregion

    #region P/I

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setActive(int id, bool state);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void release(int id);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern void createScript(int id, string name);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern void createComponent(int id, string name);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern void removeComponent(int id, string name);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern void instantiate(int id, int newId, int parentId);

    #endregion
}
