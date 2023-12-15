namespace EraScriptingCore.Domain.Components;

public interface ICollidable
{
    void OnCollisionEnter(EEntity collision);
    void OnTriggerEnter(EEntity trigger);
}
