// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "dx/dx.h"
#include "dx/dx_descriptor.h"

namespace era_engine
{
	struct dx_command_list;
	struct dx_root_signature;

	struct dx_dynamic_descriptor_heap
	{
	public:
		void initialize(uint32 numDescriptorsPerHeap = 1024);

		void stageDescriptors(uint32 rootParameterIndex, uint32 offset, uint32 numDescriptors, dx_cpu_descriptor_handle srcDescriptor);

		void commitStagedDescriptorsForDraw(dx_command_list* commandList);
		void commitStagedDescriptorsForDispatch(dx_command_list* commandList);

		void setCurrentDescriptorHeap(dx_command_list* commandList);

		void parseRootSignature(const dx_root_signature& rootSignature);

		void reset();

	private:
		NODISCARD com<ID3D12DescriptorHeap> requestDescriptorHeap();
		NODISCARD com<ID3D12DescriptorHeap> createDescriptorHeap();

		NODISCARD uint32 computeStaleDescriptorCount() const;

		void commitStagedDescriptors(dx_command_list* commandList, bool graphics);

		struct descriptor_table_cache
		{
			descriptor_table_cache()
				: numDescriptors(0)
				, baseDescriptor(nullptr)
			{}

			// Reset the table cache.
			void reset()
			{
				numDescriptors = 0;
				baseDescriptor = nullptr;
			}

			uint32 numDescriptors;
			D3D12_CPU_DESCRIPTOR_HANDLE* baseDescriptor;
		};

		static const uint32 maxDescriptorTables = 32;

		uint32 numDescriptorsPerHeap;

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> descriptorHandleCache;
		descriptor_table_cache descriptorTableCache[maxDescriptorTables];

		// Each bit in the bit mask represents the index in the root signature that contains a descriptor table.
		uint32 descriptorTableBitMask;
		uint32 staleDescriptorTableBitMask;

		std::vector<com<ID3D12DescriptorHeap>> descriptorHeapPool;
		std::vector<com<ID3D12DescriptorHeap>> freeDescriptorHeaps;

		com<ID3D12DescriptorHeap> currentDescriptorHeap;
		dx_gpu_descriptor_handle currentGPUDescriptorHandle;
		dx_cpu_descriptor_handle currentCPUDescriptorHandle;

		uint32 numFreeHandles;
	};
}