using EraScriptingCore.Core;
using EraScriptingCore.Extensions;
using System.Numerics;
using System.Runtime.CompilerServices;
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
        if (args.Length == 1)
        {
            Type = (RigidbodyType)args[0];
            initializeRigidbody(Entity.Id, (uint)Type);
        }
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

    public unsafe Vector3 GetLinearVelocity() 
    {
        Debug.Log("SYKA");
        float* ptr = (float*)getLinearVelocity(Entity.Id);
        Vector3 velocity = new Vector3(*ptr, *(++ptr), *(++ptr));
        return velocity;
    }

    public Vector3 GetAngularVelocity()
    {
        //IntPtr ptr = getAngularVelocity(Entity.Id);
        //Vector3 velocity = Memory.PtrToStruct<Vector3>(ptr);
        //Memory.ReleaseIntPtr(ptr);
        //return velocity;
        return new Vector3();
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
