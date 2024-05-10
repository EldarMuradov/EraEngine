using EraEngine.Components;

namespace EraEngine;

[AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
public class RequireComponentAttribute<T> : Attribute where T : EComponent
{
}
