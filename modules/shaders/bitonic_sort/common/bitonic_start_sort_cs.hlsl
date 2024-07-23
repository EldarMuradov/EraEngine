#include "cs.hlsli"
#include "bitonic_sort_rs.hlsli"
#include "indirect.hlsli"

ConstantBuffer<bitonic_sort_cb> cb						: register(b0);
StructuredBuffer<uint> counterBuffer					: register(t0);
RWStructuredBuffer<D3D12_DISPATCH_ARGUMENTS> dispatch	: register(u0);

uint nextPowerOfTwo(uint i)
{
	uint mask = (1u << firstbithigh(i)) - 1;
	return (i + mask) & ~mask;
}

[numthreads(22, 1, 1)]
[RootSignature(BITONIC_SORT_RS)]
void main(cs_input IN)
{
	if (IN.groupIndex >= cb.maxNumIterations)
	{
		return;
	}

	uint listCount = counterBuffer[cb.counterOffset];
	uint k = 2048u << IN.groupIndex;

	// We need one more iteration every time the number of thread groups doubles.
	if (k > nextPowerOfTwo((listCount + 2047) & ~2047))
	{
		listCount = 0;
	}

	uint offset = IN.groupIndex * (IN.groupIndex + 1) / 2;

	// Generate outer sort dispatch arguments.
	for (uint j = k / 2; j > 1024; j /= 2)
	{
		// All of the groups of size 2j that are full.
		uint completeGroups = (listCount & ~(2 * j - 1)) / 2048;

		// Remaining items must only be sorted if there are more than j of them.
		uint partialGroups = ((uint)max(int(listCount - completeGroups * 2048 - j), 0) + 1023) / 1024;

		D3D12_DISPATCH_ARGUMENTS arguments = { completeGroups + partialGroups, 1, 1 };
		dispatch[offset] = arguments;

		++offset;
	}

	// The inner sort always sorts all groups (rounded up to multiples of 2048).
	D3D12_DISPATCH_ARGUMENTS arguments = { (listCount + 2047) / 2048, 1, 1 };
	dispatch[offset] = arguments;
}
