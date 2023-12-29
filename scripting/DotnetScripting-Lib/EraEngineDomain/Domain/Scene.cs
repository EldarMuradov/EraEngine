namespace EraEngine;

public static class Scene
{
    public static Dictionary<int, EEntity> Entities = new();

    public static void Add(EEntity entity) 
    {
        if (!Entities.TryAdd(entity.Id, entity))
            Entities[entity.Id] = entity;
    }
}
