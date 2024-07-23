#include "cs.hlsli"
#include "bitonic_sort_rs.hlsli"

ConstantBuffer<bitonic_sort_cb> cb		    : register(b0);
StructuredBuffer<uint> counterBuffer	    : register(t0);
RWStructuredBuffer<uint> sortBuffer         : register(u0);

#if defined(BITONIC_SORT_FLOAT)
RWStructuredBuffer<float> comparisonBuffer  : register(u1);
#endif

groupshared key_value gs_sortKeys[2048];

void loadKeyIndexPair(uint element, uint listCount)
{
#if defined(BITONIC_SORT_UINT)
    gs_sortKeys[element & 2047] = (element < listCount ? sortBuffer[element] : cb.nullItem);
#elif defined(BITONIC_SORT_FLOAT)
    key_value kv = {
        element < listCount ? comparisonBuffer[element] : cb.nullItem,
        element < listCount ? sortBuffer[element] : 0
    };
    gs_sortKeys[element & 2047] = kv;
#endif
}

void storeKeyIndexPair(uint element, uint listCount)
{
    if (element < listCount)
    {
#if defined(BITONIC_SORT_UINT)
        sortBuffer[element] = gs_sortKeys[element & 2047];
#elif defined(BITONIC_SORT_FLOAT)
        comparisonBuffer[element] = gs_sortKeys[element & 2047].key;
        sortBuffer[element] = gs_sortKeys[element & 2047].value;
#endif
    }
}

[numthreads(1024, 1, 1)]
[RootSignature(BITONIC_SORT_RS)]
void main(cs_input IN)
{
    uint listCount = counterBuffer[cb.counterOffset];

    // Item index of the start of this group.
    const uint groupStart = IN.groupID.x * 2048;

    // Load from memory into LDS to prepare sort.
    loadKeyIndexPair(groupStart + IN.groupIndex, listCount);
    loadKeyIndexPair(groupStart + IN.groupIndex + 1024, listCount);

    GroupMemoryBarrierWithGroupSync();

#if defined(BITONIC_SORT_FLOAT)
    bool ascending = cb.nullItem > 0.f;
#endif

    // This is better unrolled because it reduces ALU and because some
    // architectures can load/store two LDS items in a single instruction
    // as long as their separation is a compile-time constant.
    [unroll]
    for (uint j = 1024; j > 0; j /= 2)
    {
        uint index2 = insertOneBit(IN.groupIndex, j);
        uint index1 = index2 ^ j;

        key_value a = gs_sortKeys[index1];
        key_value b = gs_sortKeys[index2];

        if (
#if defined(BITONIC_SORT_UINT)
            shouldSwapUint(a, b, cb.nullItem)
#elif defined(BITONIC_SORT_FLOAT)
            shouldSwapFloat(a.key, b.key, ascending)
#endif                
            )
        {
            // Swap the keys.
            gs_sortKeys[index1] = b;
            gs_sortKeys[index2] = a;
        }

        GroupMemoryBarrierWithGroupSync();
    }

    storeKeyIndexPair(groupStart + IN.groupIndex, listCount);
    storeKeyIndexPair(groupStart + IN.groupIndex + 1024, listCount);
}