using EraEngine.Extensions;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace EraEngine.Components;

public enum ForceMode : byte
{
    None,
    Force,
    Impulse,
    VelocityChange,
    Acceleration
}

public enum RigidbodyType : byte
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
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => _mass;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
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

    public RigidbodyType Type 
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private set;
    }

    #region Core Logic

    public void AddForce(Vector3 force, ForceMode mode) 
    {
        IntPtr vec = Memory.StructToIntPtr(force);
        addForce(Entity.Id, (byte)mode, vec);
        Memory.ReleaseIntPtr(vec);
    }

    public void AddTorque(Vector3 torque, ForceMode mode)
    {
        IntPtr vec = Memory.StructToIntPtr(torque);
        addTorque(Entity.Id, (byte)mode, vec);
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
        if (args.Length != 1)
            throw new ArgumentException("Wrong number of arguments!");
        Type = (RigidbodyType)args[0];
        initializeRigidbody(Entity.Id, (byte)Type);
        
    }

    #region P/I

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void addForce(int id, byte mode, IntPtr force);

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void addTorque(int id, byte mode, IntPtr torque);

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
    private static extern void initializeRigidbody(int id, byte type);

    #endregion
}
