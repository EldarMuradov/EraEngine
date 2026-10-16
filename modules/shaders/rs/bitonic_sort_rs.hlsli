#ifndef BITONIC_SORT_RS_HLSLI
#define BITONIC_SORT_RS_HLSLI

struct BitonicSortCB
{
	uint32 max_num_iterations;
	uint32 counter_offset;
#if defined(BITONIC_SORT_UINT)
	uint32 null_item;
#elif defined(BITONIC_SORT_FLOAT)
	float null_item;
#elif !defined(HLSL)
	union
	{
		uint32 null_uint;
		float null_float;
	};
#endif
};

struct BitonicSortKjCB
{
	uint32 k; // k >= 4096.
	uint32 j; // j >= 2048 && j < k.
};

#define BITONIC_SORT_RS \
	"RootConstants(num32BitConstants=3, b0), " \
    "UAV(u0), " \
    "UAV(u1), " \
    "SRV(t0), " \
	"RootConstants(num32BitConstants=2, b1)"

#define BITONIC_SORT_RS_CB					0
#define BITONIC_SORT_RS_DISPATCH			1 // For start shader.
#define BITONIC_SORT_RS_SORT_BUFFER			1 // For the others.
#define BITONIC_SORT_RS_COMPARISON_BUFFER	2 // If sorting with separate float comparison buffer.
#define BITONIC_SORT_RS_COUNTER_BUFFER		3
#define BITONIC_SORT_RS_KJ					4

#ifdef HLSL

#if defined(BITONIC_SORT_UINT)
#define KeyValue uint
#elif defined(BITONIC_SORT_FLOAT)
struct KeyValue
{
	float key;
	uint value;
};
#endif

// Takes Value and widens it by one bit at the location of the bit
// in the mask.  A one is inserted in the space. OneBitMask must
// have one and only one bit set.
static inline uint insert_one_bit(uint value, uint one_bit_mask)
{
	uint mask = one_bit_mask - 1;
	return (value & ~mask) << 1 | (value & mask) | one_bit_mask;
}

// Determines if two sort keys should be swapped in the list.  NullItem is
// either 0 or 0xffffffff.  XOR with the NullItem will either invert the bits
// (effectively a negation) or leave the bits alone.  When the the NullItem is
// 0, we are sorting descending, so when A < B, they should swap.  For an
// ascending sort, ~A < ~B should swap.
static inline bool should_swap_uint(uint a, uint b, uint null_item)
{
    return (a ^ null_item) < (b ^ null_item);
}

static inline bool should_swap_float(float a, float b, bool ascending)
{
	return (a > b) == ascending;
}

//static inline bool shouldSwapHalf(uint a, uint b, bool ascending)
//{
//	float af = f16tof32(a >> 16);
//	float bf = f16tof32(b >> 16);
//	return shouldSwapFloat(af, bf, ascending);
//}

#endif


#endif
