#include "cs.hlsli"
#include "bitonic_sort_rs.hlsli"

ConstantBuffer<bitonic_sort_cb> cb		: register(b0);
ConstantBuffer<bitonic_sort_kj_cb> kj	: register(b1);
StructuredBuffer<uint> counterBuffer	: register(t0);
RWStructuredBuffer<uint> sortBuffer     : register(u0);

#if defined(BITONIC_SORT_FLOAT)
RWStructuredBuffer<float> comparisonBuffer  : register(u1);
#endif

[numthreads(1024, 1, 1)]
[RootSignature(BITONIC_SORT_RS)]
void main(cs_input IN)
{
    uint listCount = counterBuffer[cb.counterOffset];

    // Form unique index pair from dispatch thread ID.
    uint k = kj.k;
    uint j = kj.j;
    uint index2 = insertOneBit(IN.dispatchThreadID.x, j);
    uint index1 = index2 ^ (k == 2 * j ? k - 1 : j);

    if (index2 >= listCount)
    {
        return;
    }

#if defined(BITONIC_SORT_UINT)
    key_value a = sortBuffer[index1];
    key_value b = sortBuffer[index2];

    if (shouldSwapUint(a, b, cb.nullItem))
    {
        sortBuffer[index1] = b;
        sortBuffer[index2] = a;
    }
#elif defined(BITONIC_SORT_FLOAT)

    bool ascending = cb.nullItem > 0.f;

    float a = comparisonBuffer[index1];
    float b = comparisonBuffer[index2];

    if (shouldSwapFloat(a, b, ascending))
    {
        uint aVal = sortBuffer[index1];
        uint bVal = sortBuffer[index2];

        sortBuffer[index1] = bVal;
        sortBuffer[index2] = aVal;

        comparisonBuffer[index1] = b;
        comparisonBuffer[index2] = a;
    }
#endif
}
