namespace EraEngine;

[AttributeUsage(AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Field, AllowMultiple = false, Inherited = false)]
public class HideInInspectorAttribute : Attribute
{
}
