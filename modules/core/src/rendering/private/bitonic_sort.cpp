// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/bitonic_sort.h"

#include "core/random.h"

#include "dx/dx_pipeline.h"
#include "dx/dx_command_list.h"
#include "dx/dx_barrier_batcher.h"
#include "dx/dx_profiling.h"

#include "bitonic_sort_rs.hlsli"

namespace era_engine
{
    static DXPipeline start_sort_pipeline;

    // Uint
    static DXPipeline pre_sort_uint_pipeline;
    static DXPipeline inner_sort_uint_pipeline;
    static DXPipeline outer_sort_uint_pipeline;

    // Float
    static DXPipeline pre_sort_float_pipeline;
    static DXPipeline inner_sort_float_pipeline;
    static DXPipeline outer_sort_float_pipeline;

    static ref<DXBuffer> dispatch_buffer;

    void initialize_bitonic_sort()
    {
        start_sort_pipeline = create_reloadable_pipeline("bitonic_start_sort_cs");

        // Uint
        pre_sort_uint_pipeline = create_reloadable_pipeline("bitonic_pre_sort_uint_cs");
        inner_sort_uint_pipeline = create_reloadable_pipeline("bitonic_inner_sort_uint_cs");
        outer_sort_uint_pipeline = create_reloadable_pipeline("bitonic_outer_sort_uint_cs");

        // Float
        pre_sort_float_pipeline = create_reloadable_pipeline("bitonic_pre_sort_float_cs");
        inner_sort_float_pipeline = create_reloadable_pipeline("bitonic_inner_sort_float_cs");
        outer_sort_float_pipeline = create_reloadable_pipeline("bitonic_outer_sort_float_cs");

        dispatch_buffer = create_buffer(sizeof(D3D12_DISPATCH_ARGUMENTS), 22 * 23 / 2, 0, true, false);
    }

    static std::pair<uint32, uint32> get_aligned_max_num_elements_and_num_iterations(uint32 max_num_elements)
    {
        uint32 aligned_max_num_elements = align_to_power_of_two(max_num_elements);
        uint32 max_num_iterations = log2(max(2048u, aligned_max_num_elements)) - 10;

        return {aligned_max_num_elements, max_num_iterations};
    }

    static void bitonic_sort_internal(DXCommandList* cl,
        const ref<DXBuffer>& sort_buffer, uint32 sort_buffer_offset,
        const ref<DXBuffer>& comparison_buffer, uint32 comparison_buffer_offset,
        const ref<DXBuffer>& counter_buffer, BitonicSortCB cb, uint32 aligned_max_num_elements,
        const DXPipeline& pre_sort_pipeline, const DXPipeline& outer_sort_pipeline, const DXPipeline& inner_sort_pipeline)
    {
        DX_PROFILE_BLOCK(cl, "Bitonic sort");

        // ----------------------------------------
        // START
        // ----------------------------------------
        {
            DX_PROFILE_BLOCK(cl, "Start");

            cl->set_pipeline_state(*start_sort_pipeline.pipeline);
            cl->set_compute_root_signature(*start_sort_pipeline.root_signature);

            cl->set_compute_32bit_constants(BITONIC_SORT_RS_CB, cb);
            cl->set_root_compute_uav(BITONIC_SORT_RS_DISPATCH, dispatch_buffer);
            cl->set_root_compute_srv(BITONIC_SORT_RS_COUNTER_BUFFER, counter_buffer);

            cl->dispatch(1);
            BarrierBatcher(cl)
                //.uav(dispatchBuffer)
                .transition(dispatch_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        }

        // ----------------------------------------
        // PRE-SORT
        // ----------------------------------------

        // Pre-sort the buffer up to k = 2048. This also pads the list with invalid indices
        // that will drift to the end of the sorted list.

        // Since the root signatures between all stages are identical, we don't need to switch between them and don't need to rebind the root arguments.
        // The only thing we need to bind here is the sort buffer, since this is not needed in the start stage.

        {
            DX_PROFILE_BLOCK(cl, "Pre sort");

            cl->set_pipeline_state(*pre_sort_pipeline.pipeline);

            cl->set_root_compute_uav(BITONIC_SORT_RS_SORT_BUFFER, sort_buffer->gpu_virtual_address + sort_buffer_offset);
            if (comparison_buffer)
            {
                cl->set_root_compute_uav(BITONIC_SORT_RS_COMPARISON_BUFFER, comparison_buffer->gpu_virtual_address + comparison_buffer_offset);
            }

            cl->dispatch_indirect(1, dispatch_buffer, 0);

            BarrierBatcher(cl)
                .uav(sort_buffer)
                .uav(comparison_buffer);
        }

        // ----------------------------------------
        // SORT
        // ----------------------------------------
        uint32 indirect_args_offset = 12;

        for (uint32 k = 4096; k <= aligned_max_num_elements; k *= 2)
        {
            DX_PROFILE_BLOCK(cl, "Outer sort");

            cl->set_pipeline_state(*outer_sort_pipeline.pipeline);

            for (uint32 j = k / 2; j >= 2048; j /= 2)
            {
                cl->set_compute_32bit_constants(BITONIC_SORT_RS_KJ, BitonicSortKjCB{k, j});
                cl->dispatch_indirect(1, dispatch_buffer, indirect_args_offset);

                BarrierBatcher(cl)
                    .uav(sort_buffer)
                    .uav(comparison_buffer);

                indirect_args_offset += 12;
            }

            DX_PROFILE_BLOCK(cl, "Inner sort");

            cl->set_pipeline_state(*inner_sort_pipeline.pipeline);

            cl->dispatch_indirect(1, dispatch_buffer, indirect_args_offset);

            BarrierBatcher(cl)
                .uav(sort_buffer)
                .uav(comparison_buffer);

            indirect_args_offset += 12;
        }

        cl->transition_barrier(dispatch_buffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    void bitonic_sort_uint(DXCommandList* cl,
        const ref<DXBuffer>& sort_keys_and_values, uint32 sort_offset, uint32 max_num_elements,
        const ref<DXBuffer>& counter_buffer, uint32 counter_buffer_offset, bool sort_ascending)
    {
        ASSERT(sort_offset % sizeof(uint32) == 0);
        ASSERT(counter_buffer_offset % sizeof(uint32) == 0);

        auto [aligned_max_num_elements, max_num_iterations] = get_aligned_max_num_elements_and_num_iterations(max_num_elements);
        BitonicSortCB cb;
        cb.max_num_iterations = max_num_iterations;
        cb.counter_offset = counter_buffer_offset / sizeof(uint32);
        cb.null_uint = sort_ascending ? 0xffffffff : 0;

        bitonic_sort_internal(cl, sort_keys_and_values, sort_offset, 0, 0,
            counter_buffer, cb, aligned_max_num_elements, pre_sort_uint_pipeline, outer_sort_uint_pipeline, inner_sort_uint_pipeline);
    }

    void bitonic_sort_float(DXCommandList* cl,
        const ref<DXBuffer>& sort_keys, uint32 sort_key_offset,
        const ref<DXBuffer>& sort_values, uint32 sort_value_offset, uint32 max_num_elements,
        const ref<DXBuffer>& counter_buffer, uint32 counter_buffer_offset, bool sort_ascending)
    {
        ASSERT(sort_key_offset % sizeof(float) == 0);
        ASSERT(sort_value_offset % sizeof(uint32) == 0);
        ASSERT(counter_buffer_offset % sizeof(uint32) == 0);

        auto [aligned_max_num_elements, max_num_iterations] = get_aligned_max_num_elements_and_num_iterations(max_num_elements);
        BitonicSortCB cb;
        cb.max_num_iterations = max_num_iterations;
        cb.counter_offset = counter_buffer_offset / sizeof(uint32);
        cb.null_float = sort_ascending ? FLT_MAX : -FLT_MAX;

        bitonic_sort_internal(cl, sort_values, sort_value_offset, sort_keys, sort_key_offset,
            counter_buffer, cb, aligned_max_num_elements, pre_sort_float_pipeline, outer_sort_float_pipeline, inner_sort_float_pipeline);
    }

    void test_bitonic_sort_uint(uint32 num_elements, bool ascending)
    {
        uint32 mask = align_to_power_of_two(num_elements) - 1;

        RandomNumberGenerator rng = {125912842};

        uint32* values = new uint32[num_elements];
        for (uint32 i = 0; i < num_elements; ++i)
        {
            values[i] = (rng.random_uint32() & ~mask) | i;
        }

        ref<DXBuffer> list = create_buffer(sizeof(uint32), num_elements, values, true);
        ref<DXBuffer> counter = create_buffer(sizeof(uint32), 1, &num_elements);

        delete[] values;

        ref<DXBuffer> dispatch_readback = create_readback_buffer(sizeof(D3D12_DISPATCH_ARGUMENTS), 22 * 23 / 2);
        ref<DXBuffer> list_readback = create_readback_buffer(sizeof(uint32), num_elements);

        DXCommandList* cl = dx_context.get_free_render_command_list();

        bitonic_sort_uint(cl, list, 0, num_elements, counter, 0, ascending);
        cl->transition_barrier(list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cl->transition_barrier(dispatch_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cl->copy_resource(dispatch_buffer->resource, dispatch_readback->resource);
        cl->copy_resource(list->resource, list_readback->resource);

        dx_context.execute_command_list(cl);
        dx_context.flush_application();

        uint32* sorted_values = (uint32*)map_buffer(list_readback, true);
        D3D12_DISPATCH_ARGUMENTS* args = (D3D12_DISPATCH_ARGUMENTS*)map_buffer(dispatch_readback, true);

        for (uint32 i = 0; i < num_elements - 1; ++i)
        {
            if ((sorted_values[i] & mask) >= num_elements)
            {
                std::cerr << "Corrupted list index detected.\n";
            }

            if (ascending)
            {
                if (sorted_values[i] > sorted_values[i + 1])
                {
                    std::cerr << "Invalid sort order:  non-ascending.\n";
                }
            }
            else
            {
                if (sorted_values[i] < sorted_values[i + 1])
                {
                    std::cerr << "Invalid sort order:  non-descending.\n";
                }
            }
        }

        if ((sorted_values[num_elements - 1] & mask) >= num_elements)
        {
            std::cerr << "Corrupted list index detected.\n";
        }

        unmap_buffer(dispatch_readback, false);
        unmap_buffer(list_readback, false);
    }

    void test_bitonic_sort_float(uint32 num_elements, bool ascending)
    {
        RandomNumberGenerator rng = {125912842};

        uint32* values = new uint32[num_elements];
        float* keys = new float[num_elements];
        for (uint32 i = 0; i < num_elements; ++i)
        {
            values[i] = i;
            keys[i] = rng.random_float_between(-10000.f, 10000.f);
        }

        ref<DXBuffer> values_list = create_buffer(sizeof(uint32), num_elements, values, true);
        ref<DXBuffer> keys_list = create_buffer(sizeof(float), num_elements, keys, true);
        ref<DXBuffer> counter = create_buffer(sizeof(uint32), 1, &num_elements);

        ref<DXBuffer> dispatch_readback = create_readback_buffer(sizeof(D3D12_DISPATCH_ARGUMENTS), 22 * 23 / 2);
        ref<DXBuffer> values_list_readback = create_readback_buffer(sizeof(uint32), num_elements);
        ref<DXBuffer> keys_list_readback = create_readback_buffer(sizeof(float), num_elements);

        DXCommandList* cl = dx_context.get_free_render_command_list();

        bitonic_sort_float(cl, keys_list, 0, values_list, 0, num_elements, counter, 0, ascending);
        cl->transition_barrier(values_list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cl->transition_barrier(keys_list, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cl->transition_barrier(dispatch_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cl->copy_resource(dispatch_buffer->resource, dispatch_readback->resource);
        cl->copy_resource(values_list->resource, values_list_readback->resource);
        cl->copy_resource(keys_list->resource, keys_list_readback->resource);

        dx_context.execute_command_list(cl);
        dx_context.flush_application();

        uint32* sorted_values = (uint32*)map_buffer(values_list_readback, true);
        float* sorted_keys = (float*)map_buffer(keys_list_readback, true);
        D3D12_DISPATCH_ARGUMENTS* args = (D3D12_DISPATCH_ARGUMENTS*)map_buffer(dispatch_readback, true);

        for (uint32 i = 0; i < num_elements - 1; ++i)
        {
            if (ascending)
            {
                if (sorted_keys[i] > sorted_keys[i + 1])
                {
                    std::cerr << "Invalid sort order:  non-ascending.\n";
                }
            }
            else
            {
                if (sorted_keys[i] < sorted_keys[i + 1])
                {
                    std::cerr << "Invalid sort order:  non-descending.\n";
                }
            }

            uint32 value = sorted_values[i];
            float key = sorted_keys[i];
            float comp_key = keys[value];

            if (key != comp_key)
            {
                std::cerr << "Invalid pairing.\n";
            }
        }

        unmap_buffer(dispatch_readback, false);
        unmap_buffer(keys_list_readback, false);
        unmap_buffer(values_list_readback, false);

        delete[] values;
        delete[] keys;
    }

}