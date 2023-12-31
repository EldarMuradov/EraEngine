﻿using EraEngine.Components;
using System.Runtime.InteropServices;

namespace EraEngine.Core;

public sealed class CollisionHandler
{
    [UnmanagedCallersOnly(EntryPoint = "handle_collisions")]
    public static void HandleCollision(IntPtr actors1, IntPtr actors2, int size)
    {
        int[] ids1 = new int[size];
        Marshal.Copy(actors1, ids1, 0, size);

        int[] ids2 = new int[size];
        Marshal.Copy(actors2, ids2, 0, size);

        for (int i = 0; i < size; i++)
        {
            foreach (var comp in Scene.Entities[ids1[i]].Components)
            {
                if (comp.Value is ICollidable collidable)
                    collidable.OnCollisionEnter(Scene.Entities[ids2[i]]);
            }

            foreach (var comp in Scene.Entities[ids2[i]].Components)
            {
                if (comp.Value is ICollidable collidable)
                    collidable.OnCollisionEnter(Scene.Entities[ids2[i]]);
            }
        }

        Marshal.FreeHGlobal(actors1);
        Marshal.FreeHGlobal(actors2);
    }
}
