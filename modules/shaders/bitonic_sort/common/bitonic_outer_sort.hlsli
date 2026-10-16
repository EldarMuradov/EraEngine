#include "cs.hlsli"
#include "bitonic_sort_rs.hlsli"

ConstantBuffer<BitonicSortCB> cb		: register(b0);
ConstantBuffer<BitonicSortKjCB> kj	: register(b1);
StructuredBuffer<uint> counter_buffer	: register(t0);
RWStructuredBuffer<uint> sort_buffer     : register(u0);

#if defined(BITONIC_SORT_FLOAT)
RWStructuredBuffer<float> comparison_buffer  : register(u1);
#endif

[numthreads(1024, 1, 1)]
[RootSignature(BITONIC_SORT_RS)]
void main(CSInput IN)
{
    uint list_count = counter_buffer[cb.counter_offset];

    // Form unique index pair from dispatch thread ID.
    uint k = kj.k;
    uint j = kj.j;
    uint index2 = insert_one_bit(IN.dispatch_thread_id.x, j);
    uint index1 = index2 ^ (k == 2 * j ? k - 1 : j);

    if (index2 >= list_count)
    {
        return;
    }

#if defined(BITONIC_SORT_UINT)
    KeyValue a = sort_buffer[index1];
    KeyValue b = sort_buffer[index2];

    if (should_swap_uint(a, b, cb.null_item))
    {
        sort_buffer[index1] = b;
        sort_buffer[index2] = a;
    }
#elif defined(BITONIC_SORT_FLOAT)

    bool ascending = cb.null_item > 0.f;

    float a = comparison_buffer[index1];
    float b = comparison_buffer[index2];

    if (should_swap_float(a, b, ascending))
    {
        uint a_val = sort_buffer[index1];
        uint b_val = sort_buffer[index2];

        sort_buffer[index1] = b_val;
        sort_buffer[index2] = a_val;

        comparison_buffer[index1] = b;
        comparison_buffer[index2] = a;
    }
#endif
}
