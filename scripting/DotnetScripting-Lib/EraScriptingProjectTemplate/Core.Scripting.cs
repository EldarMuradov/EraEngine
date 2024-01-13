using EraEngine;
using EraEngine.Components;
using EraEngine.Core;
using System.Runtime.InteropServices;
using static EraEngine.Core.RuntimeHelper;

namespace EraScriptingProjectTemplate
{
    public class Core
    {
        public static CreateComponentFunc CreateComp = () => new TestScript();

        #region P/I

        [UnmanagedCallersOnly(EntryPoint = "InitializeScripting")]
        public static void InitializeScripting()
        {
            Debug.Log("Usage of debug from user dll");
        }

        [UnmanagedCallersOnly(EntryPoint = "GetTestScriptCreationFunc")]
        public static void GetTestScriptCreationFunc(int id)
        {
            Debug.Log("Usage of debug from user dll");
            Scene.GetEntity(id).CreateComponent<TestScript>();
        }

        #endregion
    }
}
