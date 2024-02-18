using System.Collections.Immutable;
namespace EraEngine;

using EEntityFilter = Int32;

public static class EWorld
{
    public static Dictionary<int, EEntity> Entities = [];

    private static Dictionary<int, EEntity> _entities = [];

    private static Dictionary<EEntityFilter, List<EEntity>> _filterEntities = [];

    private static Dictionary<int, HashSet<int>> _includeUpdateSets = [];
    private static Dictionary<int, HashSet<int>> _excludeUpdateSets = [];
    private static FiltersCollection _filtersCollection = new();
    private static Dictionary<int, BitMask> _masks = [];

    public static EEntity GetEntity(int id)
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

    private static void AddIdToFlters(int id, HashSet<int> filterIds)
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

    private static void AddFilterToUpdateSets(in BitMask components, int filterIdx,
        Dictionary<int, HashSet<int>> sets)
    {
        var nextSetBit = components.GetEnumerator().GetNextSetBit(0);
        while (nextSetBit != -1)
        {
            if (!sets.TryGetValue(nextSetBit, out HashSet<int>? value))
            {
                value = new HashSet<int>(EcsCacheSettings.UpdateSetSize);
                sets.Add(nextSetBit, value);
            }

            value.Add(filterIdx);

            nextSetBit = components.GetEnumerator().GetNextSetBit(nextSetBit + 1);
        }
    }

    public static int RegisterFilter(in BitMask includes, in BitMask excludes)
    {
        if (_filtersCollection.TryAdd(in includes, in excludes, out int filterId))
        {
            var filter = _filtersCollection[filterId];
            AddFilterToUpdateSets(in filter.Includes, filterId, _includeUpdateSets);
            AddFilterToUpdateSets(in filter.Excludes, filterId, _excludeUpdateSets);
        }

        return filterId;
    }

    private static void RemoveIdFromFilters(int id, HashSet<int> filterIds)
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

    public static EcsFilter GetFilter(int id) => _filtersCollection[id];

    public static void UpdateFiltersOnAdd<T>(int id)
    {
        var componentId = ComponentMeta<T>.Id;

        if (_excludeUpdateSets.TryGetValue(componentId, out HashSet<int>? value))
            RemoveIdFromFilters(id, value);

        _masks[id] = _masks[id].Set(componentId);
        if (!_includeUpdateSets.TryGetValue(componentId, out HashSet<int>? value1))
        {
            value1 = new HashSet<int>(EcsCacheSettings.UpdateSetSize);
            _includeUpdateSets.Add(componentId, value1);
        }

        AddIdToFlters(id, value1);
    }

    public static void UpdateFiltersOnRemove(int componentId, int id)
    {
        RemoveIdFromFilters(id, _includeUpdateSets[componentId]);

        _masks[id] = _masks[id].Unset(componentId);

        if (!_excludeUpdateSets.TryGetValue(componentId, out HashSet<int>? value))
        {
            value = new HashSet<int>(EcsCacheSettings.UpdateSetSize);
            _excludeUpdateSets.Add(componentId, value);
        }

        AddIdToFlters(id, value);
    }

    public static void Add(EEntity entity)
    {
        if (_entities.TryAdd(entity.Id, entity))
        {
            if (_filterEntities.TryGetValue(entity.Filter, out List<EEntity>? value))
                value.Add(entity);
            else
                _filterEntities.Add(entity.Filter, new(16) {entity});

            _masks.Add(entity.Id, new BitMask());
        }
    }

    public static void RefreshScene()
    {
        var entities = _entities.Values.Where(e => e.Filter == -1).ToImmutableList();

        foreach (var entity in entities)
            _entities.Remove(entity.Id);

        SyncEntities();
    }

    public static void Remove(EEntity entity)
    {
        if (_entities.Remove(entity.Id))
        {
            if (_filterEntities.TryGetValue(entity.Filter, out List<EEntity>? value))
                value.Remove(entity);
        }
    }

    public static IEnumerable<EEntity>? GetFilterEntities(EEntityFilter filter)
    {
        if(_filterEntities.TryGetValue(filter, out var entities))
            return entities.AsEnumerable();
        return Enumerable.Empty<EEntity>();
    }

    public static void IterateAll(Action<EEntity> action)
    { 
        var entities = Entities.Values;
        Parallel.ForEach(entities, action);
    }

    public static void Iterate(Action<EEntity> action, EEntityFilter filter)
    {
        if (_filterEntities.TryGetValue(filter, out List<EEntity>? entities))
            Parallel.ForEach(entities, action);
    }

    public static void Iterate(Action<EEntity> action, EcsFilter filter)
    {
        foreach (var id in filter)
        {
            var entity = GetEntity(id);
            action(entity);
        }
    }

    public static void SyncEntities()
    {
        Entities = new(_entities);
    }
}
