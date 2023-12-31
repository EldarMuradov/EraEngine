﻿using System.Runtime.InteropServices;

namespace EraEngine.Components;

public sealed class CapsuleCollider: Collider
{
    internal override void InitializeComponentInternal(params object[] args)
    {
        if (args.Length < 2)
            throw new ArgumentException("Runtime> You must put at least 2 arguments!");

        Radius = (float)args[0];
        Height = (float)args[1];
        Type = ColliderType.Capsule;
        initializeCapsuleCollider(Entity.Id, Radius, Height);
    }

    public float Radius { get; private set; }

    public float Height { get; private set; }

    [DllImport("EraScriptingCPPDecls.dll")]
    private static extern void initializeCapsuleCollider(int id, float radius, float height);
}
