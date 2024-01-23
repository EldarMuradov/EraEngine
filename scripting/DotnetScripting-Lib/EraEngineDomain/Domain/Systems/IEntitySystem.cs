namespace EraEngine;

public interface IEntitySystem : IESystem
{
    List<EEntity> Entities { get; }
}
