#include "cs.hlsli"
#include "bitonic_sort_rs.hlsli"
#include "indirect.hlsli"

ConstantBuffer<BitonicSortCB> cb						: register(b0);
StructuredBuffer<uint> counter_buffer					: register(t0);
RWStructuredBuffer<D3D12_DISPATCH_ARGUMENTS> dispatch	: register(u0);

uint next_power_of_two(uint i)
{
	uint mask = (1u << firstbithigh(i)) - 1;
	return (i + mask) & ~mask;
}

[numthreads(22, 1, 1)]
[RootSignature(BITONIC_SORT_RS)]
void main(CSInput IN)
{
	if (IN.group_index >= cb.max_num_iterations)
	{
		return;
	}

	uint list_count = counter_buffer[cb.counter_offset];
	uint k = 2048u << IN.group_index;

	// We need one more iteration every time the number of thread groups doubles.
	if (k > next_power_of_two((list_count + 2047) & ~2047))
	{
		list_count = 0;
	}

	uint offset = IN.group_index * (IN.group_index + 1) / 2;

	// Generate outer sort dispatch arguments.
	for (uint j = k / 2; j > 1024; j /= 2)
	{
		// All of the groups of size 2j that are full.
		uint complete_groups = (list_count & ~(2 * j - 1)) / 2048;

		// Remaining items must only be sorted if there are more than j of them.
		uint partial_groups = ((uint)max(int(list_count - complete_groups * 2048 - j), 0) + 1023) / 1024;

		D3D12_DISPATCH_ARGUMENTS arguments = { complete_groups + partial_groups, 1, 1 };
		dispatch[offset] = arguments;

		++offset;
	}

	// The inner sort always sorts all groups (rounded up to multiples of 2048).
	D3D12_DISPATCH_ARGUMENTS arguments = { (list_count + 2047) / 2048, 1, 1 };
	dispatch[offset] = arguments;
}
