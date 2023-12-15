namespace EraScriptingCore.Domain.Components;

public abstract class EComponent
{
    public EEntity Entity = null!;

    public virtual void Start() { Console.WriteLine("Start"); }
    public virtual void Update(float dt) { Console.WriteLine("Update"); }

    public virtual void Initialize(params object[] args) { }
}
