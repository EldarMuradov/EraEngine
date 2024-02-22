using System.Collections.Immutable;
using System.Runtime.CompilerServices;
namespace EraEngine;

using EEntityFilter = Int32;

public static class EWorld
{
    public static World SceneWorld
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private set;
    } = new();

    public static void SetWorld(in World world)
    {
        SceneWorld.Clear();
        SceneWorld = world;
    }
}

public class World
{
    public Dictionary<int, EEntity> Entities = [];

    private Dictionary<int, EEntity> _entities = [];

    private Dictionary<EEntityFilter, List<EEntity>> _filterEntities = [];

    private Dictionary<int, HashSet<int>> _includeUpdateSets = [];
    private Dictionary<int, HashSet<int>> _excludeUpdateSets = [];
    private FiltersCollection _filtersCollection = new();

    private Dictionary<int, BitMask> _masks = [];

    public World()
    {

    }

    public EEntity GetEntity(int id)
    {
        if (_entities.TryGetValue(id, out EEntity? value))
            return value;
        else
        {
            var entity = new EEntity(id, "New Entity");
            _entities[id] = entity;
            return entity;
        }
    }

    private void AddIdToFlters(int id, HashSet<int> filterIds)
    {
        foreach (var filterId in filterIds)
        {
            var filter = _filtersCollection[filterId];

            var pass = _masks[id].InclusivePass(filter.Includes);
            pass &= _masks[id].ExclusivePass(filter.Excludes);

            if (!pass)
                continue;

            filter.Add(id);
        }
    }

    public void Clear()
    {
        Entities.Clear();

        _entities.Clear();

        _masks.Clear();

        _excludeUpdateSets.Clear();
        _includeUpdateSets.Clear();

        _filterEntities.Clear();

        _filtersCollection = new();

        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();
    }

    private void AddFilterToUpdateSets(in BitMask components, int filterIdx,
        Dictionary<int, HashSet<int>> sets)
    {
        var nextSetBit = components.GetEnumerator().GetNextSetBit(0);
        while (nextSetBit != -1)
        {
            if (!sets.TryGetValue(nextSetBit, out HashSet<int>? value))
            {
                value = new HashSet<int>(EcsCacheSettings.UpdateSetSize);
                sets.TryAdd(nextSetBit, value);
            }

            value.Add(filterIdx);

            nextSetBit = components.GetEnumerator().GetNextSetBit(nextSetBit + 1);
        }
    }

    public int RegisterFilter(in BitMask includes, in BitMask excludes)
    {
        if (_filtersCollection.TryAdd(in includes, in excludes, out int filterId))
        {
            var filter = _filtersCollection[filterId];
            AddFilterToUpdateSets(in filter.Includes, filterId, _includeUpdateSets);
            AddFilterToUpdateSets(in filter.Excludes, filterId, _excludeUpdateSets);
        }

        return filterId;
    }

    private void RemoveIdFromFilters(int id, HashSet<int> filterIds)
    {
        foreach (var filterId in filterIds)
        {
            var filter = _filtersCollection[filterId];

            var pass = _masks[id].InclusivePass(filter.Includes);
            pass &= _masks[id].ExclusivePass(filter.Excludes);

            if (!pass)
                continue;

            filter.Remove(id);
        }
    }

    public EcsFilter GetFilter(int id) => _filtersCollection[id];

    public void UpdateFiltersOnAdd<T>(int id)
    {
        var componentId = ComponentMeta<T>.Id;

        if (_excludeUpdateSets.TryGetValue(componentId, out HashSet<int>? filter))
            RemoveIdFromFilters(id, filter);

        _masks[id] = _masks[id].Set(componentId);
        if (!_includeUpdateSets.TryGetValue(componentId, out HashSet<int>? value))
        {
            value = new HashSet<int>(EcsCacheSettings.UpdateSetSize);
            _includeUpdateSets.TryAdd(componentId, value);
        }

        AddIdToFlters(id, value);
    }

    public void UpdateFiltersOnRemove(int componentId, int id)
    {
        RemoveIdFromFilters(id, _includeUpdateSets[componentId]);

        _masks[id] = _masks[id].Unset(componentId);

        if (!_excludeUpdateSets.TryGetValue(componentId, out HashSet<int>? value))
        {
            value = new HashSet<int>(EcsCacheSettings.UpdateSetSize);
            _excludeUpdateSets.TryAdd(componentId, value);
        }

        AddIdToFlters(id, value);
    }

    public void Add(EEntity entity)
    {
        if (_entities.TryAdd(entity.Id, entity))
        {
            if (_filterEntities.TryGetValue(entity.Filter, out List<EEntity>? value))
                value.Add(entity);
            else
                _filterEntities.TryAdd(entity.Filter, [entity]);

            _masks.TryAdd(entity.Id, new BitMask());
        }
    }

    public void RefreshScene()
    {
        var entities = _entities.Values.Where(e => e.Filter == -1).ToImmutableList();

        foreach (var entity in entities)
            _entities.Remove(entity.Id);

        SyncEntities();
    }

    public void Remove(EEntity entity)
    {
        if (_entities.Remove(entity.Id))
        {
            if (_filterEntities.TryGetValue(entity.Filter, out List<EEntity>? value))
                value.Remove(entity);
        }
    }

    public IEnumerable<EEntity>? GetFilterEntities(EEntityFilter filter)
    {
        if(_filterEntities.TryGetValue(filter, out var entities))
            return entities.AsEnumerable();
        return Enumerable.Empty<EEntity>();
    }

    public void IterateAll(Action<EEntity> action)
    { 
        var entities = Entities.Values;
        Parallel.ForEach(entities, action);
    }

    public void Iterate(Action<EEntity> action, EEntityFilter filter)
    {
        if (_filterEntities.TryGetValue(filter, out List<EEntity>? entities))
            Parallel.ForEach(entities, action);
    }

    public void Iterate(Action<EEntity> action, EcsFilter filter)
    {
        foreach (var id in filter)
        {
            var entity = GetEntity(id);
            action(entity);
        }
    }

    public void SyncEntities()
    {
        Entities = new(_entities);
    }
}
