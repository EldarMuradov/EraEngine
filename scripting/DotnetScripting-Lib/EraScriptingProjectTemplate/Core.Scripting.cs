using EraEngine.Core;
using System.Runtime.InteropServices;

namespace EraScriptingProjectTemplate
{
    public class Core
    {
        #region P/I

        [UnmanagedCallersOnly(EntryPoint = "InitializeScripting")]
        public static unsafe void InitializeScripting()
        {
            Debug.Log("Usage of debug from user dll");
        }

        #endregion
    }
}
