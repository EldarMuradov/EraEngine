using System.Runtime.CompilerServices;

namespace EraEngine;

public class EList<T>
{
    public T[] Elements;
    public int End = 0;

    public int Length
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => End;
    }

    public int Reserved
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => Elements.Length;
    }

    public ref T this[int i]
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => ref Elements[i];
    }

    public EList(int reserved = 0)
    {
        Elements = new T[reserved];
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void Copy(in EList<T> other)
    {
        End = other.End;
        if (Elements.Length < End)
            Array.Resize(ref Elements, other.Elements.Length);
        Array.Copy(other.Elements, Elements, End);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void Remove(int idx)
    {
        Elements[idx] = default;
        End--;
        if (idx < End)
            Elements[idx] = Elements[End];
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void Clear()
    {
        End = 0;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void Add(T element)
    {
        if (End >= Elements.Length)
        {
            var newLength = Elements.Length > 0 ? Elements.Length * 2 : 2;
            while (End >= newLength)
                newLength *= 2;
            Array.Resize(ref Elements, newLength);
        }
        Elements[End] = element;
        End++;
    }
}
