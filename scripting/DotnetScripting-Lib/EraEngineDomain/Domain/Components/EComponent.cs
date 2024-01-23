namespace EraEngine.Components;

public abstract class EComponent
{
    public EEntity Entity = null!;

    public virtual void Start() { /*Debug.Log("Start");*/ }
    public virtual void Update(float dt) { /*Debug.Log("Update");*/ }

    public virtual void OnCollisionEnter(EEntity collision) { Debug.Log("Collision with " + collision.Id.ToString()); }

    public virtual void OnTriggerEnter(EEntity trigger) { }

    internal abstract void InitializeComponentInternal(params object[] args);
}
