using System.Runtime.InteropServices;

namespace EraEngine;

public class EEntityManager : IEManager
{
    public static int CreateEntity(string name) => createEntity(name);

    [DllImport("EraScriptingCPPDecls.dll", CharSet = CharSet.Ansi)]
    private static extern int createEntity(string name);
}
