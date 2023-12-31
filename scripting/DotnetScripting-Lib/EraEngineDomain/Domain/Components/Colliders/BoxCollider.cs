﻿using System.Runtime.InteropServices;

namespace EraEngine.Components;

public sealed class BoxCollider : Collider
{
    internal override void InitializeComponentInternal(params object[] args)
    {
        if (args.Length < 3)
            throw new ArgumentException("Runtime> You must put at least 3 arguments!");
        
        Height = (float)args[0];
        Length = (float)args[1];
        Width = (float)args[2];
        Type = ColliderType.Box;
        initializeBoxCollider(Entity.Id, Height, Length, Width);
    }

    /// <summary>
    /// X
    /// </summary>
    public float Height { get; private set; }

    /// <summary>
    /// Y
    /// </summary>
    public float Length { get; private set; }

    /// <summary>
    /// Z
    /// </summary>
    public float Width { get; private set; }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void initializeBoxCollider(int id, float x, float y, float z);
}
