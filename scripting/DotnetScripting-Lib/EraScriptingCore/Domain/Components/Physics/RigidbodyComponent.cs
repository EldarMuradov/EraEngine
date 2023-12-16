using EraScriptingCore.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraScriptingCore.Domain.Components;

public enum ForceMode : uint
{
    None,
    Force,
    Impulse
}

public enum RigidbodyType : uint
{
    None,
    Static,
    Dynamic,
    Kinematic
}

public sealed class RigidbodyComponent : EComponent
{
    public override void Initialize(params object[] args)
    {
        Type = (RigidbodyType)args[0];
    }

    public float Mass 
    {
        get => _mass;
        set 
        {
            if (value >= 0)
            { 
                _mass = value;
                setMass(Entity.Id, _mass);
            }
            else
                throw new InvalidOperationException("Physics> Mass is negative!");
        }
    }

    private float _mass = 1.0f;

    public RigidbodyType Type { get; private set; }

    #region Core Logic

    public void AddForce(Vector3 force, ForceMode mode) 
    {
        IntPtr vec = Memory.StructToIntPtr(force);
        addForce(Entity.Id, (uint)mode, vec);
        Memory.ReleaseIntPtr(vec);
    }

    public Vector3 GetLinearVelocity() 
    {
        return getLinearVelocity(Entity.Id);
    }

    public Vector3 GetAngularVelocity()
    {
        return getAngularVelocity(Entity.Id);
    }

    public void SetLinearVelocity(Vector3 velocity)
    {
        IntPtr vec = Memory.StructToIntPtr(velocity);
        setLinearVelocity(Entity.Id, vec);
        Memory.ReleaseIntPtr(vec);
    }

    public void SetAngularVelocity(Vector3 velocity)
    {
        IntPtr vec = Memory.StructToIntPtr(velocity);
        setAngularVelocity(Entity.Id, vec);
        Memory.ReleaseIntPtr(vec);
    }

    #endregion

    #region P/I

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void addForce(int id, uint mode, IntPtr force);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern Vector3 getLinearVelocity(int id);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern Vector3 getAngularVelocity(int id);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setLinearVelocity(int id, IntPtr velocity);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setAngularVelocity(int id, IntPtr velocity);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern float getMass(int id);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setMass(int id, float mass);

    #endregion
}
