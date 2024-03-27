namespace EraEngine.Components;

public abstract class EComponent
{
    public EEntity Entity = null!;

    public virtual void Start() { /*Debug.Log("Start");*/ }
    public virtual void Update(float dt) { /*Debug.Log("Update");*/ }

    public virtual void OnCollisionEnter(EEntity collision) { Debug.Log($"Collision with {collision.Id}"); }

    public virtual void OnCollisionExit(EEntity collision) { Debug.Log($"Collision exit with {collision.Id}"); }

    public virtual void OnTriggerEnter(EEntity trigger) { }
    public virtual void OnTriggerExit(EEntity trigger) { }

    public static void Signal(string signal) { ESystemManager.GetSystem<EventSystem>().Handle(new EventRequest { Name = signal }); }

    public static bool IsSignaled(string signal) { return ESystemManager.GetSystem<EventSystem>().Signaled(signal); }

    internal virtual void InitializeComponentInternal(params object[] args) { }
}
