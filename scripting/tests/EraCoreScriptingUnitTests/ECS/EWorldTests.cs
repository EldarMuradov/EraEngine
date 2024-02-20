using EraEngine;
using EraEngine.Components;

namespace EraCoreScriptingUnitTests.ECS;

public class EWorldTests
{
    private static readonly SemaphoreSlim _semaphore = new(1, 1);

    [Fact]
    public async Task RegisterAndGetFilterTest()
    {
        try
        {
            // Capture
            await _semaphore.WaitAsync();
            EWorld.Clear();

            BitMask include = new();
            include.Set(ComponentMeta<TransformComponent>.Id);

            BitMask exclude = new();
            exclude.Set(ComponentMeta<MeshRendererComponent>.Id);

            const int validEnttId = 2;

            // Act
            var filterId = EWorld.RegisterFilter(include, exclude);

            EEntity e1 = new(1, "test1");
            e1.CreateComponent<MeshRendererComponent>();

            EEntity e2 = new(validEnttId, "test2");

            EWorld.SyncEntities();

            // Assert
            var filter = EWorld.GetFilter(filterId);
            EWorld.Iterate((entity) => { Assert.Equal(validEnttId, entity.Id); }, filter);
        }
        finally
        { 
            // Free
            EWorld.Clear();
            _semaphore.Release();
        }
    }

    [Fact]
    public async Task CreateAndGetEntityByIdTest()
    {
        try
        {
            // Capture
            await _semaphore.WaitAsync();
            EWorld.Clear();

            const int validEnttId = 3;

            EEntity e = new(validEnttId, "test3");

            // Act
            EWorld.SyncEntities();

            // Assert
            Assert.Equal(validEnttId, EWorld.GetEntity(validEnttId).Id);
        }
        finally
        {
            // Free
            EWorld.Clear();
            _semaphore.Release();
        }
    }

    [Fact]
    public async Task CheckFilterCreationForEntities()
    {
        try
        {
            // Capture
            await _semaphore.WaitAsync();
            EWorld.Clear();

            const int filterId = 2;
            const int expectedCount = 2;

            EEntity e1 = new(1, "test1");
            EEntity e2 = new(2, "test2");
            EEntity e3 = new(3, "test3", filterId);
            EEntity e4 = new(4, "test3", filterId);

            // Act
            EWorld.SyncEntities();

            // Assert
            Assert.Equal(expectedCount, EWorld.GetFilterEntities(filterId)?.Count());
        }
        finally
        {
            // Free
            EWorld.Clear();
            _semaphore.Release();
        }
    }

    [Fact]
    public async Task CreateAndRemoveEntityTest()
    {
        try
        {
            // Capture
            await _semaphore.WaitAsync();
            EWorld.Clear();

            EEntity e = new(1, "test1");

            // Act
            EWorld.SyncEntities();
            EWorld.Remove(e);
            EWorld.SyncEntities();

            // Assert
            Assert.Empty(EWorld.Entities);
        }
        finally
        {
            // Free
            EWorld.Clear();
            _semaphore.Release();
        }
    }


    [Fact]
    public async Task RefreshSceneTest()
    {
        try
        {
            // Capture
            await _semaphore.WaitAsync();
            EWorld.Clear();

            EEntity e1 = new(4, "test4");

            EEntity e2 = new(5, "test5", -1);

            EEntity e3 = new(6, "test6");

            EEntity e4 = new(7, "test7", -1);

            const int numAfterRefreshing = 2;

            // Act
            EWorld.SyncEntities();
            EWorld.RefreshScene();

            // Assert
            Assert.Equal(numAfterRefreshing, EWorld.Entities.Count);
        }
        finally
        {
            // Free
            EWorld.Clear();
            _semaphore.Release();
        }
    }
}