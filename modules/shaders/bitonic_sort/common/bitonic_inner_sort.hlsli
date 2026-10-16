#include "cs.hlsli"
#include "bitonic_sort_rs.hlsli"

ConstantBuffer<BitonicSortCB> cb		    : register(b0);
StructuredBuffer<uint> counter_buffer	    : register(t0);
RWStructuredBuffer<uint> sort_buffer         : register(u0);

#if defined(BITONIC_SORT_FLOAT)
RWStructuredBuffer<float> comparison_buffer  : register(u1);
#endif

groupshared KeyValue gs_sortKeys[2048];

void load_key_index_pair(uint element, uint list_count)
{
#if defined(BITONIC_SORT_UINT)
    gs_sortKeys[element & 2047] = (element < list_count ? sort_buffer[element] : cb.null_item);
#elif defined(BITONIC_SORT_FLOAT)
    KeyValue kv = {
        element < list_count ? comparison_buffer[element] : cb.null_item,
        element < list_count ? sort_buffer[element] : 0
    };
    gs_sortKeys[element & 2047] = kv;
#endif
}

void store_key_index_pair(uint element, uint list_count)
{
    if (element < list_count)
    {
#if defined(BITONIC_SORT_UINT)
        sort_buffer[element] = gs_sortKeys[element & 2047];
#elif defined(BITONIC_SORT_FLOAT)
        comparison_buffer[element] = gs_sortKeys[element & 2047].key;
        sort_buffer[element] = gs_sortKeys[element & 2047].value;
#endif
    }
}

[numthreads(1024, 1, 1)]
[RootSignature(BITONIC_SORT_RS)]
void main(CSInput IN)
{
    uint list_count = counter_buffer[cb.counter_offset];

    // Item index of the start of this group.
    const uint group_start = IN.group_id.x * 2048;

    // Load from memory into LDS to prepare sort.
    load_key_index_pair(group_start + IN.group_index, list_count);
    load_key_index_pair(group_start + IN.group_index + 1024, list_count);

    GroupMemoryBarrierWithGroupSync();

#if defined(BITONIC_SORT_FLOAT)
    bool ascending = cb.null_item > 0.f;
#endif

    // This is better unrolled because it reduces ALU and because some
    // architectures can load/store two LDS items in a single instruction
    // as long as their separation is a compile-time constant.
    [unroll]
    for (uint j = 1024; j > 0; j /= 2)
    {
        uint index2 = insert_one_bit(IN.group_index, j);
        uint index1 = index2 ^ j;

        KeyValue a = gs_sortKeys[index1];
        KeyValue b = gs_sortKeys[index2];

        if (
#if defined(BITONIC_SORT_UINT)
            should_swap_uint(a, b, cb.null_item)
#elif defined(BITONIC_SORT_FLOAT)
            should_swap_float(a.key, b.key, ascending)
#endif                
            )
        {
            // Swap the keys.
            gs_sortKeys[index1] = b;
            gs_sortKeys[index2] = a;
        }

        GroupMemoryBarrierWithGroupSync();
    }

    store_key_index_pair(group_start + IN.group_index, list_count);
    store_key_index_pair(group_start + IN.group_index + 1024, list_count);
}