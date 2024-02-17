using System.Runtime.CompilerServices;

namespace EraEngine.Configuration;

public class Version(int build, int release)
{
    public int Release
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        init;
    } = build;

    public int Build
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        init;
    } = release;

    public static bool operator ==(Version left, Version right) 
    { 
        return left.Build == right.Build && left.Release == right.Release;
    }

    public static bool operator !=(Version left, Version right)
    {
        return left.Build != right.Build && left.Release != right.Release;
    }

    public static bool operator >(Version left, Version right) 
    {
        if (left.Release > right.Release)
            return true;

        if (left.Release < right.Release)
            return false;

        return left.Build > right.Build;
    }

    public static bool operator <(Version left, Version right)
    {
        if (left.Release < right.Release)
            return true;

        if (left.Release < right.Release)
            return false;

        return left.Build < right.Build;
    }

    public override bool Equals(object? obj)
    {
        if (ReferenceEquals(this, obj))
            return true;

        return false;
    }

    public bool Equals(Version obj)
    {
        return this == obj;
    }

    public override int GetHashCode()
    {
        return Build.GetHashCode() | Release.GetHashCode();
    }
}
