// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "dx/dx_dynamic_descriptor_heap.h"
#include "dx/dx_command_list.h"
#include "dx/dx_context.h"

namespace era_engine
{
	void DXDynamicDescriptorHeap::initialize(uint32 num_descriptors_per_heap)
	{
		this->num_descriptors_per_heap = num_descriptors_per_heap;
		descriptor_table_bit_mask = 0;
		stale_descriptor_table_bit_mask = 0;
		current_cpu_descriptor_handle = D3D12_DEFAULT;
		current_gpu_descriptor_handle = D3D12_DEFAULT;
		num_free_handles = 0;

		descriptor_handle_cache.resize(num_descriptors_per_heap);
	}

	void DXDynamicDescriptorHeap::stage_descriptors(uint32 root_parameter_index, uint32 offset, uint32 num_descriptors, DXCPUDescriptorHandle src_descriptor)
	{
		ASSERT(num_descriptors <= num_descriptors_per_heap && root_parameter_index < max_descriptor_tables);

		DescriptorTableCache& cache = descriptor_table_cache[root_parameter_index];

		ASSERT((offset + num_descriptors) <= cache.num_descriptors);

		D3D12_CPU_DESCRIPTOR_HANDLE* dst_descriptor = (cache.base_descriptor + offset);
		for (uint32 i = 0; i < num_descriptors; ++i)
		{
			dst_descriptor[i] = src_descriptor + i;
		}

		setBit(stale_descriptor_table_bit_mask, root_parameter_index);
	}

	void DXDynamicDescriptorHeap::commit_staged_descriptors(DXCommandList* command_list, bool graphics)
	{
		uint32 num_descriptors_to_commit = compute_stale_descriptor_count();

		if (num_descriptors_to_commit > 0)
		{
			if (!current_descriptor_heap || num_free_handles < num_descriptors_to_commit)
			{
				current_descriptor_heap = request_descriptor_heap();
				current_cpu_descriptor_handle = current_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
				current_gpu_descriptor_handle = current_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
				num_free_handles = num_descriptors_per_heap;

				command_list->set_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, current_descriptor_heap);

				// When updating the descriptor heap on the command list, all descriptor
				// tables must be (re)recopied to the new descriptor heap (not just
				// the stale descriptor tables).
				stale_descriptor_table_bit_mask = descriptor_table_bit_mask;
			}

			DWORD root_index;
			while (_BitScanForward(&root_index, stale_descriptor_table_bit_mask))
			{
				uint32 num_src_descriptors = descriptor_table_cache[root_index].num_descriptors;
				D3D12_CPU_DESCRIPTOR_HANDLE* src_descriptor_handles = descriptor_table_cache[root_index].base_descriptor;
				D3D12_CPU_DESCRIPTOR_HANDLE dest_descriptor_range_start = current_cpu_descriptor_handle;

				dx_context.device->CopyDescriptors(
					1, &dest_descriptor_range_start, &num_src_descriptors,
					num_src_descriptors, src_descriptor_handles, nullptr,
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

				if (graphics)
				{
					command_list->set_graphics_descriptor_table(root_index, current_gpu_descriptor_handle);
				}
				else
				{
					command_list->set_compute_descriptor_table(root_index, current_gpu_descriptor_handle);
				}

				current_cpu_descriptor_handle += num_src_descriptors;
				current_gpu_descriptor_handle += num_src_descriptors;
				num_free_handles -= num_src_descriptors;

				unsetBit(stale_descriptor_table_bit_mask, root_index);
			}
		}
	}

	void DXDynamicDescriptorHeap::commit_staged_descriptors_for_draw(DXCommandList* command_list)
	{
		commit_staged_descriptors(command_list, true);
	}

	void DXDynamicDescriptorHeap::commit_staged_descriptors_for_dispatch(DXCommandList* command_list)
	{
		commit_staged_descriptors(command_list, false);
	}

	void DXDynamicDescriptorHeap::set_current_descriptor_heap(DXCommandList* command_list)
	{
		command_list->set_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, current_descriptor_heap);
	}

	void DXDynamicDescriptorHeap::parse_root_signature(const DXRootSignature& root_signature)
	{
		stale_descriptor_table_bit_mask = 0;

		descriptor_table_bit_mask = root_signature.table_root_parameter_mask;
		uint32 num_descriptor_tables = root_signature.num_descriptor_tables;

		uint32 bitmask = descriptor_table_bit_mask;
		uint32 current_offset = 0;
		DWORD root_index;
		uint32 descriptor_table_index = 0;
		while (_BitScanForward(&root_index, bitmask))
		{
			uint32 num_descriptors = root_signature.descriptor_table_sizes[descriptor_table_index++];

			DescriptorTableCache& cache = descriptor_table_cache[root_index];
			cache.num_descriptors = num_descriptors;
			cache.base_descriptor = &descriptor_handle_cache[current_offset];

			current_offset += num_descriptors;

			// Flip the descriptor table bit so it's not scanned again for the current index.
			unsetBit(bitmask, root_index);
		}

		ASSERT(current_offset <= num_descriptors_per_heap); // The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap.
	}

	void DXDynamicDescriptorHeap::reset()
	{
		free_descriptor_heaps = descriptor_heap_pool;
		current_descriptor_heap.Reset();
		current_cpu_descriptor_handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
		current_gpu_descriptor_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
		num_free_handles = 0;
		descriptor_table_bit_mask = 0;
		stale_descriptor_table_bit_mask = 0;

		// Reset the table cache.
		for (int i = 0; i < max_descriptor_tables; ++i)
		{
			descriptor_table_cache[i].reset();
		}
	}

	NODISCARD com<ID3D12DescriptorHeap> DXDynamicDescriptorHeap::request_descriptor_heap()
	{
		com<ID3D12DescriptorHeap> descriptor_heap;
		if (free_descriptor_heaps.size() > 0)
		{
			descriptor_heap = free_descriptor_heaps.back();
			free_descriptor_heaps.pop_back();
		}
		else
		{
			descriptor_heap = create_descriptor_heap();
			descriptor_heap_pool.push_back(descriptor_heap);
		}

		return descriptor_heap;
	}

	NODISCARD com<ID3D12DescriptorHeap> DXDynamicDescriptorHeap::create_descriptor_heap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
		descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		descriptor_heap_desc.NumDescriptors = num_descriptors_per_heap;
		descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		com<ID3D12DescriptorHeap> descriptor_heap;
		checkResult(dx_context.device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap)));

		return descriptor_heap;
	}

	NODISCARD uint32 DXDynamicDescriptorHeap::compute_stale_descriptor_count() const
	{
		uint32 num_stale_descriptors = 0;
		DWORD i;
		DWORD stale_descriptors_bit_mask = stale_descriptor_table_bit_mask;

		while (_BitScanForward(&i, stale_descriptors_bit_mask))
		{
			num_stale_descriptors += descriptor_table_cache[i].num_descriptors;
			unsetBit(stale_descriptors_bit_mask, i);
		}

		return num_stale_descriptors;
	}

}