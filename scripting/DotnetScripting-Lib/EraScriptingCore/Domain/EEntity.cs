using EraScriptingCore.Core;
using EraScriptingCore.Domain.Components;
using System.ComponentModel;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Domain;

public class EEntity
{
    public EEntity(int id, string name)
    {
        (Id, Name) = (id, name);
        CreateComponentInternal<TransformComponent>();
    }

    public int Id { get; init; }

    public string Name { get; init; }

    public Dictionary<string, EComponent> Components = new();

    public EEntity? Parent;

    public List<EEntity> Childs = new();

    public bool ActiveSelf
    {
        get
        {
            return _activeSelf;
        }
        set
        {
            _activeSelf = value;
            setActive(Id, _activeSelf);
        }
    }

    private bool _activeSelf = true;

    #region Pipeline Methods

    public void Start()
    {
        foreach (var comp in Components)
            comp.Value.Start();
    }

    public void Update(float dt)
    {
        foreach (var comp in Components)
            comp.Value.Update(dt);
    }

    #endregion

    #region  Core Logic

    public T? GetComponent<T>() where T : EComponent, new()
    {
        if (Components.TryGetValue(nameof(T), out var comp))
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
            throw new InvalidOperationException($"Scripting> Failed to create {nameof(T)} component!");

        comp.Initialize(args);

        if (comp is Script)
            createScript(Id, nameof(T));
        else
            createComponent(Id, nameof(T)); // TODO: agrs sync

        Components.Add(nameof(T), comp);
        comp.Entity = this;

        comp.Start();

        return comp;
    }

    public void RemoveComponent<T>(bool sync = false) where T : EComponent, new()
    {
        var comp = GetComponent<T>() ?? 
            throw new NullReferenceException("Runtime> Failed to remove component! Value is null.");
        
        var compname = nameof(T);

        Components.Remove(compname);
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

        foreach (var comp in original.Components)
            instance.CopyComponent(comp.Value);

        var transform = instance.GetComponent<TransformComponent>()!;
        transform.SetPosition(position);
        transform.SetRotation(rotation);

        Scene.Additional.Add(newId, instance);

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

        foreach (var comp in original.Components)
            instance.CopyComponent(comp.Value);

        Scene.Additional.Add(newId, instance);

        return instance;
    }

    public void AddChild(EEntity entity)
    {
        Childs.Add(entity);
    }

    public EComponent CopyComponent(EComponent component)
    {
        var type = component.GetType();

        foreach (var c in Components)
        {
            if (c.Value.GetType() == type)
                return c.Value;
        }

        var comp = component;

        Components.Add(comp.GetType().Name, comp);
        comp.Entity = this;

        return comp;
    }

    public void RemoveChild(EEntity entity)
    {
        Childs.Remove(entity);
    }

    public void Release() => release(Id);

    #endregion

    #region Internal

    public T CreateComponentInternal<T>(params object[] args) where T : EComponent, new()
    {
        var comp = GetComponent<T>();

        if (comp != null)
            return comp;

        comp = new T();

        if (comp is null)
            throw new InvalidOperationException($"Scripting> Failed to create {nameof(T)} component!");

        comp.Initialize(args);

        Components.Add(nameof(T), comp);
        comp.Entity = this;

        comp.Start();

        return comp;
    }

    internal void RemoveComponent(string name, bool sync = true)
    {
        Components.Remove(name);
        if (sync)
            removeComponent(Id, name);
    }

    internal void AddComponentFromInstance(EComponent comp, string name, bool from = true)
    {
        if (comp != null)
        {
            Components.Add(name, comp);
            comp.Entity = this;
            comp.Start();
            if (from)
                createScript(Id, name);
        }
    }

    internal static void RemoveComponentFromEngine(int id, string comp)
    {
        Scene.Entities[id].RemoveComponent(comp, false);
        Debug.Log($"Scripting> {comp} removed successfuly");
    }

    [DllImport("EraScriptingProjectTemplate.dll", EntryPoint = "GetCtors")]
    private static extern CustomTypesDTO GetCtors();

    internal static void AddComponentFromEngine(int id, string comp)
    {
        var component = GetCtors().CustomTypes[comp];
        Debug.Log($"Runtime> {comp} added successfuly");
        Scene.Entities[id].AddComponentFromInstance(component(), comp, false);
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
