using EraEngine.Components;
using EraEngine.Infrastructure;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace EraEngine;

using EEntityFilter = Int32;

public class EEntity
{
    public EEntity(int id, string name, EEntityFilter filter = default, World world = null!)
    {
        (Id, Name, Filter) = (id, name, filter);

        _world = world is null ? EWorld.SceneWorld : world;

        _world.Add(this);

        CreateComponentInternal<TransformComponent>();
    }

    public EEntity(World world = null!)
    {
        Name = Guid.NewGuid().ToString();
        Filter = default;
        Id = EEntityManager.CreateEntity(Name);

        _world = world is null ? EWorld.SceneWorld : world;

        _world.Add(this);

        CreateComponentInternal<TransformComponent>();
    }

    public EEntity(string name, EEntityFilter filter, World world = null!)
    {
        Name = name;
        Filter = filter;
        Id = EEntityManager.CreateEntity(Name);

        _world = world is null ? EWorld.SceneWorld : world;

        _world.Add(this);

        CreateComponentInternal<TransformComponent>();
    }

    private World _world = null!;

    public int Id
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        init;
    }

    public EEntityFilter Filter
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        init;
    }

    public string Name
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        init;
    } = null!;

    public EEntity? Parent;

    public List<EEntity> Childs = [];

    public ComponentsContainer Components = new();

    public bool IsInitialized
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private set;
    } = false;

    public bool ActiveSelf
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get
        {
            return _activeSelf;
        }
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
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
        IsInitialized = true;
        Components.SyncComponents();

        var components = Components.Values;
        foreach (var comp in components)
            comp.Start();
    }

    public void Update(float dt)
    {
        var components = Components.Values;
        foreach (var comp in components)
            comp.Update(dt);
        Components.SyncComponents();
    }

    #endregion

    #region  Core Logic

    public T? GetComponent<T>() where T : EComponent, new()
    {
        if (Components.TryGetValue(typeof(T).Name, out var comp))
            return (T)comp;
        return null;
    }

    public T CreateComponent<T>(params object[] args) where T : EComponent, new()
    {
        _world.UpdateFiltersOnAdd<T>(Id);
        var comp = GetComponent<T>();

        if (comp != null)
            return comp;

        comp = new T();

        if (comp is null)
        {
            Debug.LogError($"Scripting> Failed to create {nameof(T)} component!");
            return null!;
        }

        if (comp is EScript)
            createScript(Id, comp.GetType().Name);

        comp.Entity = this;
        comp.InitializeComponentInternal(args);

        Components.Add(comp.GetType().Name, comp);

        if(IsInitialized)
            comp.Start();

        return comp;
    }

    public void RemoveComponent<T>(bool sync = false) where T : EComponent, new()
    {
        _world.UpdateFiltersOnRemove(ComponentMeta<T>.Id, Id);
        var comp = GetComponent<T>() ??
            throw new NullReferenceException("Runtime> Failed to remove component! Value is null.");

        var compname = comp.GetType().Name;

        Components.Remove(compname);
        if (sync)
            removeComponent(Id, compname);
    }

    public void RemoveComponent(string name, bool sync = true)
    {
        //EWorld.UpdateFiltersOnRemove(ComponentMeta<T>.Id, Id);
        Components.Remove(name);
        if (sync)
            removeComponent(Id, name);
    }

    public static EEntity Instantiate(EEntity original, Vector3 position, Quaternion rotation, EEntity? parent = null)
    {
        int newId = EEntityManager.CreateEntity(Guid.NewGuid().ToString());

        if (parent != null)
            instantiate(original.Id, newId, parent.Id);
        else
            instantiate(original.Id, newId, -1);

        EEntity instance = new(newId, original.Name + newId.ToString());

        if (parent != null)
            instance.Parent = parent;

        foreach (var comp in original.Components.ActiveValues)
            instance.CopyComponent(comp);

        var transform = instance.GetComponent<TransformComponent>()!;
        transform.SetPosition(position);
        transform.SetRotation(rotation);

        EWorld.SceneWorld.Entities.Add(newId, instance);

        return instance;
    }

    public static EEntity Instantiate(EEntity original, EEntity? parent = null)
    {
        int newId = EEntityManager.CreateEntity(Guid.NewGuid().ToString());

        if (parent != null)
            instantiate(original.Id, newId, (int)parent.Id);
        else
            instantiate(original.Id, newId, -1);

        EEntity instance = new(newId, original.Name + newId.ToString());

        if (parent != null)
            instance.Parent = parent;

        foreach (var comp in original.Components.ActiveValues)
            instance.CopyComponent(comp);

        EWorld.SceneWorld.Entities.Add(newId, instance);

        return instance;
    }

    public void AddChild(EEntity entity)
    {
        Childs.Add(entity);
    }

    public EComponent CopyComponent(EComponent component)
    {
        if (Components.ContainsKey(component.GetType().Name))
            RemoveComponent(component.GetType().Name);
        var comp = component;
        comp.Entity = this;
        Components.Add(comp.GetType().Name, comp);

        //EWorld.UpdateFiltersOnAdd<>(Id);

        if (comp is EScript)
            createScript(Id, comp.GetType().Name);

        return comp;
    }

    public void RemoveChild(EEntity entity)
    {
        Childs.Remove(entity);
    }

    public void Release()
    {
        release(Id);
        _world.Remove(this);
    } 

    #endregion

    internal T CreateComponentInternal<T>(params object[] args) where T : EComponent, new()
    {
        _world.UpdateFiltersOnAdd<T>(Id);
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

        Components.Add(comp.GetType().Name, comp);

        if (IsInitialized)
            comp.Start();

        return comp;
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
