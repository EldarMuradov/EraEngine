using System.Runtime.CompilerServices;
namespace EraEngine;

public static class EcsCacheSettings
{
    public const int UpdateSetSize = 4;
    public const int FilteredEntitiesSize = 128;
}

public struct EcsFilter
{
    public BitMask Includes;
    public BitMask Excludes;

    public delegate void IteartionDelegate(int[] entities, int count);

    public int Length => _entitiesVector.Length;

    public int this[int i] => _entitiesVector[i];

    public EList<int> Entities
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => _entitiesVector;
    }

    public int HashCode
    {
        get
        {
            if (!_cachedHash.HasValue)
                _cachedHash = GetHashFromMasks(Includes, Excludes);
            return _cachedHash.Value;
        }
    }

    private Dictionary<int, int> _filteredEntities = [];

    private EList<int> _entitiesVector;

    private HashSet<int> _addSet = [];

    private HashSet<int> _removeSet = [];

    private BoxedInt _lockCount;

    private int? _cachedHash;

    private class BoxedInt { public int Value = 0; }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int GetHashFromMasks(in BitMask includes, in BitMask excludes)
    {
        int hash = 17;

        foreach (var bit in includes)
            hash = hash * 23 + bit.GetHashCode();

        foreach (var bit in excludes)
            hash = hash * 23 + bit.GetHashCode();

        return hash;
    }

    public EcsFilter(in BitMask includes, in BitMask excludes, bool dummy = false)
    {
        Includes = default;
        Includes.Copy(includes);

        Excludes = default;
        Excludes.Copy(excludes);

        _filteredEntities = dummy ? null : new Dictionary<int, int>(EcsCacheSettings.FilteredEntitiesSize);

        _addSet = dummy ? null : new HashSet<int>();

        _removeSet = dummy ? null : new HashSet<int>();

        _cachedHash = GetHashFromMasks(Includes, Excludes);

        _lockCount = dummy ? null : new BoxedInt();

        _entitiesVector = dummy ? null : new EList<int>(EcsCacheSettings.FilteredEntitiesSize);

        currentEntityIndex = -1;
    }

    #region Enumerable

    private int currentEntityIndex;

    public int Current
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => _entitiesVector.Elements[currentEntityIndex];
    }

    public void Lock() => _lockCount.Value++;

    public bool MoveNext()
    {
        currentEntityIndex++;
        return currentEntityIndex < _entitiesVector.End;
    }

    public void Cleanup()
    {
        currentEntityIndex = -1;
        _lockCount.Value--;

        if (_lockCount.Value == 0)
        {
            foreach (var id in _addSet)
            {
                if (!_filteredEntities.ContainsKey(id))
                {
                    _entitiesVector.Add(id);
                    _filteredEntities.Add(id, _entitiesVector.End - 1);
                }
            }

            _addSet.Clear();

            foreach (var id in _removeSet)
            {
                if (_filteredEntities.TryGetValue(id, out int idx))
                {
                    var lastEntity = _entitiesVector.Elements[_entitiesVector.End - 1];
                    _entitiesVector.Remove(idx);

                    if (idx < _entitiesVector.End)
                    {
                        _filteredEntities[lastEntity] = _filteredEntities[id];
                        _filteredEntities.Remove(id);
                    }
                    else
                        _filteredEntities.Remove(id);
                }
            }

            _removeSet.Clear();
        }
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public EcsFilter GetEnumerator() => this;

    #endregion

    public void Add(int id)
    {
        if (_lockCount.Value > 0)
            _addSet.Add(id);
        else if (!_filteredEntities.ContainsKey(id))
        {
            _entitiesVector.Add(id);
            _filteredEntities.Add(id, _entitiesVector.End - 1);
        }
    }

    public void Remove(int id)
    {
        if (_lockCount.Value > 0)
            _removeSet.Add(id);
        else if (_filteredEntities.TryGetValue(id, out int idx))
        {
            var lastEntity = _entitiesVector.Elements[_entitiesVector.End - 1];
            _entitiesVector.Remove(idx);

            if (idx < _entitiesVector.End)
            {
                _filteredEntities[lastEntity] = _filteredEntities[id];
                _filteredEntities.Remove(id);
            }
            else
                _filteredEntities.Remove(id);
        }
    }

    public void Copy(in EcsFilter other)
    {
        Includes.Copy(other.Includes);
        Excludes.Copy(other.Excludes);

        if (_filteredEntities != null)
            _filteredEntities.Clear();
        else
            _filteredEntities = new Dictionary<int, int>(EcsCacheSettings.FilteredEntitiesSize);

        foreach (var entity in other._filteredEntities)
            _filteredEntities.Add(entity.Key, entity.Value);

        _entitiesVector ??= new EList<int>(other._entitiesVector.Elements.Length);
        _entitiesVector.Copy(other._entitiesVector);

        _lockCount = other._lockCount;
    }
}
