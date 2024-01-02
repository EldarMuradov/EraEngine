namespace EraEngine;

public static class Scene
{
    public static Dictionary<int, EEntity> Entities = new();
    private static Dictionary<int, EEntity> _entities = new();

    public static EEntity GetEntity(int id) => _entities[id];

    public static void Add(EEntity entity) 
    {
        if (!_entities.TryAdd(entity.Id, entity))
            _entities[entity.Id] = entity;
    }

    public static void OnFrameEnded() 
    {
        Entities = _entities;
    }
}
