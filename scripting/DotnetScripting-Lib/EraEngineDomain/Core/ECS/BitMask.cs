using System.Runtime.CompilerServices;

namespace EraEngine;

using MaskInternal = UInt32;

public struct BitMask
{
    public const int SizeOfPartInBits = sizeof(MaskInternal) * 8;

    private MaskInternal _m1;
    private MaskInternal[] _mn;

    public int Length
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private set;
    }

    public BitMask(MaskInternal m1 = 0, MaskInternal[] mn = null!)
    {
        _m1 = m1;
        _mn = mn;
        Length = 0;
    }

    public BitMask(params int[] positions)
    {
        _m1 = 0;
        _mn = null!;
        Length = 0;

        Set(positions);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static int GetDynamicChunksLength(int length) => (int)Math.Ceiling((float)length / SizeOfPartInBits) - 1;

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void Copy(in BitMask other)
    {
        _m1 = other._m1;
        Length = other.Length;

        var otherArrLength = other._mn != null ? other._mn.Length : 0;
        if (otherArrLength > 0)
        {
            if (_mn == null || _mn.Length < otherArrLength)
                _mn = new MaskInternal[other._mn.Length];
            for (int i = 0; i < other._mn.Length; i++)
                _mn[i] = other._mn[i];
            for (int i = other._mn.Length; i < _mn.Length; i++)
                _mn[i] = 0;
        }
        else if (_mn != null)
        {
            for (int i = 0; i < _mn.Length; i++)
                _mn[i] = 0;
        }
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public BitMask Duplicate()
    {
        var copy = new BitMask();
        copy.Copy(this);
        return copy;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public BitMask Set(params int[] positions)
    {
        for (int i = 0; i < positions.Length; i++)
            Set(positions[i]);

        return this;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public BitMask Set(int i)
    {
        var chunkIdx = i / SizeOfPartInBits;
        ref var m = ref _m1;
        if (chunkIdx > 0)
        {
            chunkIdx--;

            if (_mn == null || _mn.Length <= chunkIdx)
            {
                var newChunksLength = 2;
                while (newChunksLength < chunkIdx + 1)
                    newChunksLength <<= 1;
                if (_mn == null)
                    _mn = new MaskInternal[newChunksLength];
                else
                    Array.Resize(ref _mn, newChunksLength);
            }
            m = ref _mn[chunkIdx];
        }

        int position = i % SizeOfPartInBits;
        MaskInternal shifted = 1;
        m |= shifted << position;

        i++;
        if (Length < i)
            Length = i;

        return this;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public BitMask And(int i)
    {
        var mask = this;
        mask.Set(i);
        return mask;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public BitMask And(params int[] positions)
    {
        var mask = this;
        for (int i = 0; i < positions.Length; i++)
            mask.Set(positions[i]);
        return mask;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public BitMask AndNot(int i)
    {
        var mask = this;
        mask.Unset(i);
        return mask;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public BitMask AndNot(params int[] positions)
    {
        var mask = this;
        for (int i = 0; i < positions.Length; i++)
            mask.Unset(positions[i]);

        return mask;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private bool CheckChunkIdx(int idx) => idx > 0 && (_mn == null || _mn.Length <= idx - 1);

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public BitMask Unset(int i)
    {
        int chunkIdx = i / SizeOfPartInBits;
        if (CheckChunkIdx(chunkIdx))
            return this;

        ref var m = ref _m1;
        if (chunkIdx > 0)
            m = ref _mn[chunkIdx - 1];

        int position = i % SizeOfPartInBits;
        MaskInternal shifted = 1;
        m &= ~(shifted << position);

        //update length
        if (chunkIdx == (Length - 1) / SizeOfPartInBits)
        {
            int j = chunkIdx - 1;
            var msb = 0;
            for (; j >= 0; j--)
            {
                if (_mn[j] == 0)
                    continue;
                var chunk = _mn[j];
                while (chunk != 0)
                {
                    chunk >>= 1;
                    msb++;
                }
                break;
            }
            if (j < 0)
            {
                var chunk = _m1;
                while (chunk != 0)
                {
                    chunk >>= 1;
                    msb++;
                }
            }

            j++;
            Length = j * SizeOfPartInBits + msb;
        }

        return this;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool Check(int i)
    {
        int chunkIdx = i / SizeOfPartInBits;
        if (CheckChunkIdx(chunkIdx))
            return false;

        var m = _m1;
        if (chunkIdx > 0)
            m = _mn[chunkIdx - 1];
        int position = i % SizeOfPartInBits;
        return (m & (1 << position)) != 0;
    }

    #region Enumerable

    public struct Enumerator
    {
        private int _nextSetBit;
        private BitMask _bitMask;

        public Enumerator(BitMask bitMask)
        {
            _bitMask = bitMask;
            _nextSetBit = -1;
        }

        public int Current => _nextSetBit;

        public bool MoveNext()
        {
            if (_nextSetBit == -1)
                _nextSetBit = GetNextSetBit(0);
            else
                _nextSetBit = GetNextSetBit(_nextSetBit + 1);

            return _nextSetBit != -1;
        }

        public int GetNextSetBit(int fromPosition)
        {
            for (int i = fromPosition; i < _bitMask.Length; i++)
            {
                int chunkIdx = i / SizeOfPartInBits;
                if (_bitMask.CheckChunkIdx(chunkIdx))
                    return -1;

                var m = _bitMask._m1;
                if (chunkIdx > 0)
                    m = _bitMask._mn[chunkIdx - 1];

                for (int j = i % SizeOfPartInBits; j < SizeOfPartInBits; j++)
                {
                    if ((m & (1 << j)) != 0)
                        return j + (chunkIdx * SizeOfPartInBits);
                }
            }

            return -1;
        }
    }

    public Enumerator GetEnumerator() => new Enumerator(this);

    #endregion

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void Clear()
    {
        _m1 = 0;
        if (_mn != null)
        {
            for (int i = 0; i < _mn.Length; i++)
                _mn[i] = 0;
        }
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static bool InclusivePass_Internal(MaskInternal value, MaskInternal filter) => (filter & (value ^ filter)) == 0;

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool InclusivePass(in BitMask filter)
    {
        if (filter.Length > Length)
            return false;

        if (!InclusivePass_Internal(_m1, filter._m1))
            return false;

        var chunksCount = GetDynamicChunksLength(filter.Length);
        for (int i = 0; i < chunksCount; i++)
        {
            var filterChunk = filter._mn[i];
            if (filterChunk == 0)
                continue;

            if (!InclusivePass_Internal(_mn[i], filterChunk))
                return false;
        }

        return true;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool ExclusivePass(in BitMask filter)
    {
        if ((filter._m1 & _m1) != 0)
            return false;
        if (filter._mn != null && _mn != null)
        {
            var chunksCount = GetDynamicChunksLength(filter.Length);
            for (int i = 0; i < chunksCount && i < _mn.Length; i++)
            {
                if ((filter._mn[i] & _mn[i]) != 0)
                    return false;
            }
        }

        return true;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool Equals(in BitMask other)
    {
        if (Length != other.Length)
            return false;

        if (_m1 != other._m1)
            return false;

        if (_mn != null)
        {
            int length = GetDynamicChunksLength(Length);
            for (int i = 0; i < length; i++)
            {
                if (_mn[i] != other._mn[i])
                    return false;
            }
        }

        return true;
    }
}