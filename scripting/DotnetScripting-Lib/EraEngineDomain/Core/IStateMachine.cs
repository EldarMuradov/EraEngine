namespace EraEngine;

public interface IState<T>
{
    T Data { get; }

    int State { get; }
}

public interface IStateMachine<IState>
{
    IState Current { get; }

    List<IState> States { get; }

    void SetState(IState state);

    bool MoveNext();

    void Handle();
}
