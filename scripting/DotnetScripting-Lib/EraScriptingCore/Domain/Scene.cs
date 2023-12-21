using System.Numerics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text.Json;
using EraScriptingCore.Core;
using EraScriptingCore.Domain.Components;
using EraScriptingCore.Extensions;
using EraScriptingCore.Infrastructure.Serializers;
using System.Text;
using System;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Reflection.Metadata;

namespace EraScriptingCore.Domain;

public static class Scene
{
    public static Dictionary<int, EEntity> Entities = new();

    internal static Dictionary<int, EEntity> Additional = new();

    internal static Dictionary<string, Type> Types = new();

    private static readonly SemaphoreSlim _syncObj = new(1, 1);

    [UnmanagedCallersOnly(EntryPoint = "start")]
    public static unsafe void Start()
    {
        EEntity e = new(60, "Px_Sphere");
        Debug.Log(Entities.Count.ToString());

        IntPtr ptr = CreateTestScript();
        GCHandle handle = (GCHandle)ptr;
        object obj = handle.Target;
        EComponent type = Unsafe.As<EComponent>(obj);
        pr(type);
        //var comp = (EComponent)type.GetConstructor(BindingFlags.Public, new Type[] { }).Invoke(new object[] { });
        e.Components.Add(type.GetType().Name, type);
        type.Entity = e;
        _syncObj.Wait();
        var values = Entities.Values;
        foreach (var entity in values)
            entity.Start();
        _syncObj.Release();
    }

    static void pr<T>(T t) { Console.WriteLine(t.GetType().Name); }

    [UnmanagedCallersOnly(EntryPoint = "update")]
    public static void Update(float dt)
    {
        _syncObj.Wait();
        var values = Entities.Values;
        foreach (var entity in values)
            entity.Update(dt);

        _syncObj.Release();
    }

    [DllImport("EraScriptingProjectTemplate.dll")]
    private static extern IntPtr CreateTestScript();
}
