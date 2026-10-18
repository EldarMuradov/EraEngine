// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

namespace era_engine
{
    struct DXCommandList;
    struct DXBuffer;

    void initialize_bitonic_sort();

    // These functions sort a buffer of values based on a key. The currently supported variants are:
    // - bitonicSortUint: Key and value are packed into a single uint. The sorting algorithm will simply sort this buffer. The user must pack the key into
    //	 the most significant and the value into the least significant bits (how many bits the two take is up to you).
    //   Since the values are sorted as uints, you have to make sure that your key is sortable when interpreted as a uint.
    // - bitonicSortFloat: Here you provide two buffers, one with the values (32 bit indices) and one with the keys (32 bit floats). The buffers will be sorted
    //   based on the keys.
    //
    // The input buffers for keys and values are expected to be in the resource state D3D12_RESOURCE_STATE_UNORDERED_ACCESS. They also return in this state.
    // It is expected that you have a GPU-visible buffer containing the number of elements to sort (counterBuffer). counterBufferOffset is a byte-offset into this
    // buffer to the location where the count sits.
    // The count does not need to be a power of two.

    void bitonic_sort_uint(DXCommandList* cl,
        const ref<DXBuffer>& sort_keys_and_values, uint32 sort_offset, uint32 max_num_elements,
        const ref<DXBuffer>& counter_buffer, uint32 counter_buffer_offset, bool sort_ascending);
    void bitonic_sort_float(DXCommandList* cl,
        const ref<DXBuffer>& sort_keys, uint32 sort_key_offset,
        const ref<DXBuffer>& sort_values, uint32 sort_value_offset, uint32 max_num_elements,
        const ref<DXBuffer>& counter_buffer, uint32 counter_buffer_offset, bool sort_ascending);

    void test_bitonic_sort_uint(uint32 num_elements, bool ascending);
    void test_bitonic_sort_float(uint32 num_elements, bool ascending);
}