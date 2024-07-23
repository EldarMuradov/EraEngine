using EraEngine.Extensions;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace EraEngine;

public enum CharacterControllerType 
{
    None,
    Box,
    Capsule
}

public abstract class CharacterController : EComponent
{
    public CharacterControllerType Type
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        protected set;
    }

    public virtual void Move(Vector3 position) 
    {
        IntPtr vec = Memory.StructToIntPtr(position);
        moveCCT(Entity.Id, vec);
        Memory.ReleaseIntPtr(vec);
    }

    #region P/I

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern float moveCCT(int id, IntPtr position);

    #endregion
}
