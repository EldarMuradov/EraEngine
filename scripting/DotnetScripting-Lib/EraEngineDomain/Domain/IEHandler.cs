namespace EraEngine;

public interface IEHandler
{
    void Handle() { }
}

public interface IEHandler<in TRequest> where TRequest : notnull
{
    void Handle(TRequest request);
}
