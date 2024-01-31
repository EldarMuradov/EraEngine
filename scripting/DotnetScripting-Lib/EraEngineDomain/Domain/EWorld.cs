using System.Collections.Immutable;

namespace EraEngine;

public static class EWorld
{
    public static Dictionary<int, EEntity> Entities = [];

    private static Dictionary<int, EEntity> _entities = [];

    private static Dictionary<EEntityFilter, List<EEntity>> _filterEntities = [];

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

    public static void Add(EEntity entity)
    {
        if (_entities.TryAdd(entity.Id, entity))
        {
            if (_filterEntities.TryGetValue(entity.Filter, out List<EEntity>? value))
                value.Add(entity);
            else
                _filterEntities.Add(entity.Filter, new(16) {entity});
        }
    }

    public static void RefreshScene()
    {
        var entities = _entities.Values.Where(e => e.Filter.Id == -1).ToImmutableList();

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

    public static void SyncEntities()
    {
        Entities = new(_entities);
    }
}
