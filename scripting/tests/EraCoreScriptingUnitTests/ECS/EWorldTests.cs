using EraEngine;
using EraEngine.Components;

namespace EraCoreScriptingUnitTests.ECS;

public class EWorldTests
{
    [Fact]
    public void RegisterAndGetFilterTest()
    {
        var world = new World();

        BitMask include = new();
        include.Set(ComponentMeta<TransformComponent>.Id);

        BitMask exclude = new();
        exclude.Set(ComponentMeta<MeshRendererComponent>.Id);

        const int validEnttId = 2;

        // Act
        var filterId = world.RegisterFilter(include, exclude);

        EEntity e1 = new(1, "test1", 0, world);
        e1.CreateComponent<MeshRendererComponent>();

        EEntity e2 = new(validEnttId, "test2", 0, world);

        world.SyncEntities();

        // Assert
        var filter = world.GetFilter(filterId);
        world.Iterate((entity) => { Assert.Equal(validEnttId, entity.Id); }, filter);
    }

    [Fact]
    public void CreateAndGetEntityByIdTest()
    {
        var world = new World();

        const int validEnttId = 3;

        EEntity e = new(validEnttId, "test3", 0, world);

        // Act
        world.SyncEntities();

        // Assert
        Assert.Equal(validEnttId, world.GetEntity(validEnttId).Id);
    }

    [Fact]
    public void CheckFilterCreationForEntities()
    {
        var world = new World();

        const int filterId = 2;
        const int expectedCount = 2;

        EEntity e1 = new(1, "test1", 0, world);
        EEntity e2 = new(2, "test2", 0, world);
        EEntity e3 = new(3, "test3", filterId, world);
        EEntity e4 = new(4, "test3", filterId, world);

        // Act
        world.SyncEntities();

        // Assert
        Assert.Equal(expectedCount, world.GetFilterEntities(filterId)?.Count());
    }

    [Fact]
    public void CreateAndRemoveEntityTest()
    {
        var world = new World();
        EEntity e = new(1, "test1", 0, world);

        // Act
        world.SyncEntities();
        world.Remove(e);
        world.SyncEntities();

        // Assert
        Assert.Empty(world.Entities);
    }


    [Fact]
    public void RefreshSceneTest()
    {
        var world = new World();

        EEntity e1 = new(4, "test4", 0, world);
        EEntity e2 = new(5, "test5", -1, world);
        EEntity e3 = new(6, "test6", 0, world);
        EEntity e4 = new(7, "test7", -1, world);

        const int numAfterRefreshing = 2;

        // Act
        world.SyncEntities();
        world.RefreshScene();

        // Assert
        Assert.Equal(numAfterRefreshing, world.Entities.Count);
    }
}