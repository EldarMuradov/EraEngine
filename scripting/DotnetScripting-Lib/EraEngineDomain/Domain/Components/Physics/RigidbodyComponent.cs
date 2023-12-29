using EraEngine.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Components;

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

    public unsafe Vector3 GetLinearVelocity() 
    {
        IntPtr ptrn = getLinearVelocity(Entity.Id);
        float* ptr = (float*)ptrn;
        Vector3 velocity = new(*ptr, *(ptr + 1), *(ptr + 2));
        Memory.ReleaseIntPtr(ptrn);
        return velocity;
    }

    public unsafe Vector3 GetAngularVelocity()
    {
        IntPtr ptrn = getAngularVelocity(Entity.Id);
        float* ptr = (float*)ptrn;
        Vector3 velocity = new(*ptr, *(ptr + 1), *(ptr + 2));
        Memory.ReleaseIntPtr(ptrn);
        return velocity;
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

    internal override void InitializeComponentInternal(params object[] args)
    {
        if (args.Length == 1)
        {
            Type = (RigidbodyType)args[0];
            initializeRigidbody(Entity.Id, (uint)Type);
        }
    }

    #region P/I

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void addForce(int id, uint mode, IntPtr force);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern IntPtr getLinearVelocity(int id);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern IntPtr getAngularVelocity(int id);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setLinearVelocity(int id, IntPtr velocity);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setAngularVelocity(int id, IntPtr velocity);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern float getMass(int id);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void setMass(int id, float mass);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void initializeRigidbody(int id, uint type);

    #endregion
}
