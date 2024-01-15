namespace EraEngine;

public static class Scene
{
    public static Dictionary<int, EEntity> Entities = [];

    private static Dictionary<int, EEntity> _entities = [];

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

    public static void Add(EEntity entity) => _entities.TryAdd(entity.Id, entity);

    public static void SyncEntities()
    {
        Entities = _entities;
    }
}
