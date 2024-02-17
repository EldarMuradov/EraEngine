using System.Runtime.CompilerServices;

namespace EraEngine;

class FiltersCollection
{
    class FilterEqComparer : IEqualityComparer<EcsFilter>
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool Equals(EcsFilter x, EcsFilter y) => x.Includes.Equals(y.Includes) && x.Excludes.Equals(y.Excludes);

        public int GetHashCode(EcsFilter filter) => filter.HashCode;
    }

    private HashSet<EcsFilter> _set = [];

    private List<EcsFilter> _list = [];

    private static FilterEqComparer _filterComparer;

    static FiltersCollection()
    {
        _filterComparer = new FilterEqComparer();
    }

    public int Length => _list.Count;

    public EcsFilter this[int i]
    {
        get
        {
            return _list[i];
        }
    }

    public FiltersCollection(int prealloc = 0)
    {
#if NETSTANDARD2_1_OR_GREATER || NETCOREAPP2_0_OR_GREATER || NET5_0_OR_GREATER
        _set = new HashSet<EcsFilter>(prealloc, _filterComparer);
#else
        _set = new HashSet<EcsFilter>(_filterComparer);
#endif
        _list = new List<EcsFilter>(prealloc);
    }

    public bool TryAdd(in BitMask includes, in BitMask excludes, out int idx)
    {
        var dummy = new EcsFilter(in includes, in excludes, true);
#if NETSTANDARD2_1_OR_GREATER || NETCOREAPP2_0_OR_GREATER || NET5_0_OR_GREATER
        var addNew = !_set.TryGetValue(dummy, out dummy);
#else
        var addNew = !_set.Contains(dummy);
#endif
        if (addNew)
        {
            var newFilter = new EcsFilter(in includes, in excludes);
            _set.Add(newFilter);
            _list.Add(newFilter);
            idx = _list.Count - 1;

            return true;
        }
        else
        {
#if NETSTANDARD2_1_OR_GREATER || NETCOREAPP2_0_OR_GREATER || NET5_0_OR_GREATER
            // nothing
#else
            foreach (var filter in _set)
            {
                if (!_filterComparer.Equals(dummy, filter))
                    continue;
                dummy = filter;
                break;
            }
#endif

            idx = _list.IndexOf(dummy); //TODO: This is not preformant at all
            return false;
        }
    }

    public void RemoveId(int id)
    {
        foreach (var filter in _set)
            filter.Remove(id);
    }

    public void Copy(FiltersCollection other)
    {
        // Resize
        int sz = other._list.Count;
        int cur = _list.Count;

        if (sz < cur)
            _list.RemoveRange(sz, cur - sz);
        else if (sz > cur)
        {
            if (sz > _list.Capacity)
                _list.Capacity = sz;

            for (int i = 0; i < sz - cur; i++)
                _list.Add(default);
        }

        _set.Clear();
        for (int i = 0; i < other._list.Count; i++)
        {
            var filter = _list[i];
            var otherFilter = other._list[i];
            filter.Copy(in otherFilter);

            _list[i] = filter;
            _set.Add(filter);
        }
    }
}
