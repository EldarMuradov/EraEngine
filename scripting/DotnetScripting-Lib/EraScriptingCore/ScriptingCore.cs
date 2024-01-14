using EraEngine.Core;
using EraEngineDomain.Core;

namespace EraEngine;
public delegate void Call();
public delegate void CallUpdate(float dt);
public delegate void CallHandleColls(int id1, int id2);
public delegate void CallHandleTrs(IntPtr trs, int id);

public class ScriptingCore
{
    internal static UserScripting UserScripting;

    #region P/I

    public static void Main(string[] args) 
    {
    
    }

    public static void InitializeScripting()
    {
        Debug.Log("Usage of debug");
    }

    #endregion
}
