using EraEngine.Extensions;
using System.Numerics;
using System.Runtime.InteropServices;

namespace EraEngine.Components;

public enum CharacterControllerType 
{
    None,
    Box,
    Capsule
}

public abstract class CharacterController : EComponent
{
    public CharacterControllerType Type { get; protected set; }

    public virtual void Move(Vector3 position) 
    {
        IntPtr vec = Memory.StructToIntPtr(position);
        move(Entity.Id, vec);
        Memory.ReleaseIntPtr(vec);
    }

    #region P/I

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern float move(int id, IntPtr position);

    #endregion
}
