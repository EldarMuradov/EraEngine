// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "dx/dx_buffer.h"
#include "dx/dx_command_list.h"
#include "dx/dx_context.h"

#include <dx/D3D12MemAlloc.h>

namespace era_engine
{
	DXGI_FORMAT get_index_buffer_format(uint32 element_size)
	{
		DXGI_FORMAT result = DXGI_FORMAT_UNKNOWN;
		if (element_size == 1)
		{
			result = DXGI_FORMAT_R8_UINT;
		}
		else if (element_size == 2)
		{
			result = DXGI_FORMAT_R16_UINT;
		}
		else if (element_size == 4)
		{
			result = DXGI_FORMAT_R32_UINT;
		}
		return result;
	}

	void* map_buffer(const ref<DXBuffer>& buffer, bool intents_reading, MapRange read_range)
	{
		D3D12_RANGE range = { 0, 0 };
		D3D12_RANGE* r = 0;

		if (intents_reading)
		{
			if (read_range.num_elements != -1)
			{
				range.Begin = read_range.first_element * buffer->element_size;
				range.End = range.Begin + read_range.num_elements * buffer->element_size;
				r = &range;
			}
		}
		else
		{
			r = &range;
		}

		void* result;
		buffer->resource->Map(0, r, &result);
		return result;
	}

	void unmap_buffer(const ref<DXBuffer>& buffer, bool has_written, MapRange written_range)
	{
		D3D12_RANGE range = { 0, 0 };
		D3D12_RANGE* r = 0;

		if (has_written)
		{
			if (written_range.num_elements != -1)
			{
				range.Begin = written_range.first_element * buffer->element_size;
				range.End = range.Begin + written_range.num_elements * buffer->element_size;
				r = &range;
			}
		}
		else
		{
			r = &range;
		}

		buffer->resource->Unmap(0, r);
	}

	void update_upload_buffer_data(const ref<DXBuffer>& buffer, void* data, uint32 size)
	{
		void* mapped = map_buffer(buffer, false);
		memcpy(mapped, data, size);
		unmap_buffer(buffer, true);
	}

	static void upload_buffer_data(ref<DXBuffer> buffer, const void* buffer_data)
	{
		DXCommandList* cl = dx_context.get_free_copy_command_list();

		DXResource intermediate_resource;

		auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffer->total_size);

#if !USE_D3D12_BLOCK_ALLOCATOR

		CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
		checkResult(dx_context.device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			0,
			IID_PPV_ARGS(&intermediate_resource)));
#else
		D3D12MA::ALLOCATION_DESC allocation_desc = {};
		allocation_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		D3D12MA::Allocation* allocation;
		checkResult(dx_context.memory_allocator->CreateResource(
			&allocation_desc,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			0,
			&allocation,
			IID_PPV_ARGS(&intermediate_resource)));

		dx_context.retire(allocation);
#endif

		D3D12_SUBRESOURCE_DATA subresource_data = {};
		subresource_data.pData = buffer_data;
		subresource_data.RowPitch = buffer->total_size;
		subresource_data.SlicePitch = subresource_data.RowPitch;

		cl->transition_barrier(buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		UpdateSubresources(cl->command_list.Get(),
			buffer->resource.Get(), intermediate_resource.Get(),
			0, 0, 1, &subresource_data);

		// We are omitting the transition to common here, since the resource automatically decays to common state after being accessed on a copy queue.

		dx_context.retire(intermediate_resource);
		dx_context.execute_command_list(cl);
	}

	void update_buffer_data_range(ref<DXBuffer> buffer, const void* data, uint32 offset, uint32 size)
	{
		ASSERT(offset + size <= buffer->total_size);

		DXCommandList* cl = dx_context.get_free_copy_command_list();

		DXResource intermediate_resource;

		auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);

#if !USE_D3D12_BLOCK_ALLOCATOR

		CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
		checkResult(dx_context.device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			0,
			IID_PPV_ARGS(&intermediate_resource)));
#else
		D3D12MA::ALLOCATION_DESC allocation_desc = {};
		allocation_desc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		D3D12MA::Allocation* allocation;
		checkResult(dx_context.memory_allocator->CreateResource(
			&allocation_desc,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			0,
			&allocation,
			IID_PPV_ARGS(&intermediate_resource)));

		dx_context.retire(allocation);
#endif

		cl->transition_barrier(buffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

		void* mapped;
		checkResult(intermediate_resource->Map(0, 0, &mapped));
		memcpy(mapped, data, size);
		intermediate_resource->Unmap(0, 0);

		cl->command_list->CopyBufferRegion(buffer->resource.Get(), offset, intermediate_resource.Get(), 0, size);

		// We are omitting the transition to common here, since the resource automatically decays to common state after being accessed on a copy queue.

		dx_context.retire(intermediate_resource);
		dx_context.execute_command_list(cl);
	}

	static void initialize_buffer(ref<DXBuffer> buffer, uint32 element_size, uint32 element_count, void* data, bool allow_unordered_access, bool allow_clearing, bool raytracing = false,
		D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT)
	{
		D3D12_RESOURCE_FLAGS flags = allow_unordered_access ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

		buffer->element_size = element_size;
		buffer->element_count = element_count;
		buffer->total_size = element_size * element_count;
		buffer->heap_type = heap_type;
		buffer->supports_srv = heap_type != D3D12_HEAP_TYPE_READBACK;
		buffer->supports_uav = allow_unordered_access;
		buffer->supports_clearing = allow_clearing;
		buffer->raytracing = raytracing;

		buffer->default_srv = {};
		buffer->default_uav = {};
		buffer->cpu_clear_uav = {};
		buffer->gpu_clear_uav = {};
		buffer->raytracing_srv = {};

		auto desc = CD3DX12_RESOURCE_DESC::Buffer(buffer->total_size, flags);

#if !USE_D3D12_BLOCK_ALLOCATOR

		CD3DX12_HEAP_PROPERTIES props(heap_type);
		checkResult(dx_context.device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			initial_state,
			0,
			IID_PPV_ARGS(&buffer->resource)));
#else
		D3D12MA::ALLOCATION_DESC allocation_desc = {};
		allocation_desc.HeapType = heap_type;

		D3D12MA::Allocation* allocation;
		checkResult(dx_context.memory_allocator->CreateResource(
			&allocation_desc,
			&desc,
			initial_state,
			0,
			&allocation,
			IID_PPV_ARGS(&buffer->resource)));

		buffer->allocation = allocation;
#endif

		buffer->gpu_virtual_address = buffer->resource->GetGPUVirtualAddress();

		// Upload
		if (data)
		{
			if (heap_type == D3D12_HEAP_TYPE_DEFAULT)
			{
				upload_buffer_data(buffer, data);
			}
			else if (heap_type == D3D12_HEAP_TYPE_UPLOAD)
			{
				void* data_ptr = map_buffer(buffer, false);
				memcpy(data_ptr, data, buffer->total_size);
				unmap_buffer(buffer, true);
			}
		}

		uint32 num_descriptors = buffer->supports_srv + buffer->supports_uav + buffer->supports_clearing + raytracing;
		buffer->descriptor_allocation = dx_context.srv_uav_allocator.allocate(num_descriptors);

		uint32 index = 0;

		if (buffer->supports_srv)
		{
			buffer->default_srv = DXCPUDescriptorHandle(buffer->descriptor_allocation.cpu_at(index++)).create_buffer_srv(buffer);
		}

		if (buffer->supports_uav)
		{
			buffer->default_uav = DXCPUDescriptorHandle(buffer->descriptor_allocation.cpu_at(index++)).create_buffer_uav(buffer);
		}

		if (buffer->supports_clearing)
		{
			buffer->cpu_clear_uav = DXCPUDescriptorHandle(buffer->descriptor_allocation.cpu_at(index++)).create_buffer_uint_uav(buffer);

			buffer->shader_visible_descriptor_allocation = dx_context.srv_uav_allocator_shader_visible.allocate(1);
			DXCPUDescriptorHandle(buffer->shader_visible_descriptor_allocation.cpu_at(0)).create_buffer_uint_uav(buffer);
			buffer->gpu_clear_uav = buffer->shader_visible_descriptor_allocation.gpu_at(0);
		}

		if (raytracing)
		{
			buffer->raytracing_srv = DXCPUDescriptorHandle(buffer->descriptor_allocation.cpu_at(index++)).create_raytracing_acceleration_structure_srv(buffer);
		}

		ASSERT(index == num_descriptors);
	}

	ref<DXBuffer> create_buffer(uint32 element_size, uint32 element_count, void* data, bool allow_unordered_access, bool allow_clearing, D3D12_RESOURCE_STATES initial_state)
	{
		ref<DXBuffer> result = make_ref<DXBuffer>();
		initialize_buffer(result, element_size, element_count, data, allow_unordered_access, allow_clearing, false, initial_state, D3D12_HEAP_TYPE_DEFAULT);
		return result;
	}

	ref<DXBuffer> create_upload_buffer(uint32 element_size, uint32 element_count, void* data)
	{
		ref<DXBuffer> result = make_ref<DXBuffer>();
		initialize_buffer(result, element_size, element_count, data, false, false, false, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
		return result;
	}

	ref<DXBuffer> create_readback_buffer(uint32 element_size, uint32 element_count, D3D12_RESOURCE_STATES initial_state)
	{
		ref<DXBuffer> result = make_ref<DXBuffer>();
		initialize_buffer(result, element_size, element_count, 0, false, false, false, initial_state, D3D12_HEAP_TYPE_READBACK);
		return result;
	}

	ref<DXVertexBuffer> create_vertex_buffer(uint32 element_size, uint32 element_count, void* data, bool allow_unordered_access, bool allow_clearing)
	{
		ref<DXVertexBuffer> result = make_ref<DXVertexBuffer>();
		initialize_buffer(result, element_size, element_count, data, true, allow_clearing);
		result->view.BufferLocation = result->gpu_virtual_address;
		result->view.SizeInBytes = result->total_size;
		result->view.StrideInBytes = element_size;
		return result;
	}

	ref<DXVertexBuffer> create_upload_vertex_buffer(uint32 element_size, uint32 element_count, void* data)
	{
		ref<DXVertexBuffer> result = make_ref<DXVertexBuffer>();
		initialize_buffer(result, element_size, element_count, data, false, false, false, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);
		result->view.BufferLocation = result->gpu_virtual_address;
		result->view.SizeInBytes = result->total_size;
		result->view.StrideInBytes = element_size;
		return result;
	}

	ref<DXIndexBuffer> create_index_buffer(uint32 element_size, uint32 element_count, void* data, bool allow_unordered_access, bool allow_clearing)
	{
		ref<DXIndexBuffer> result = make_ref<DXIndexBuffer>();
		initialize_buffer(result, element_size, element_count, data, allow_unordered_access, allow_clearing);
		result->view.BufferLocation = result->gpu_virtual_address;
		result->view.SizeInBytes = result->total_size;
		result->view.Format = get_index_buffer_format(element_size);
		return result;
	}

	ref<DXBuffer> create_raytracing_tlas_buffer(uint32 size)
	{
		ref<DXBuffer> result = make_ref<DXBuffer>();
		initialize_buffer(result, size, 1, nullptr, true, false, true, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
		return result;
	}

	static void retire(DXResource resource, DXDescriptorAllocation descriptor_allocation, DXDescriptorAllocation shader_visible_descriptor_allocation)
	{
		BufferGrave grave;
		grave.resource = resource;
		grave.descriptor_allocation = descriptor_allocation;
		grave.shader_visible_descriptor_allocation = shader_visible_descriptor_allocation;
		dx_context.retire(std::move(grave));
	}

	DXBuffer::~DXBuffer()
	{
		retire(resource, descriptor_allocation, shader_visible_descriptor_allocation);
		if (allocation)
		{
			dx_context.retire(allocation);
		}
	}

	void resize_buffer(ref<DXBuffer> buffer, uint32 new_element_count, D3D12_RESOURCE_STATES initial_state)
	{
		retire(buffer->resource, buffer->descriptor_allocation, buffer->shader_visible_descriptor_allocation);
		if (buffer->allocation)
		{
			dx_context.retire(buffer->allocation);
		}

		buffer->element_count = new_element_count;
		buffer->total_size = buffer->element_count * buffer->element_size;

		auto desc = buffer->resource->GetDesc();

		auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer->total_size, desc.Flags);

#if !USE_D3D12_BLOCK_ALLOCATOR

		CD3DX12_HEAP_PROPERTIES props(buffer->heap_type);
		checkResult(dx_context.device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&buffer_desc,
			initial_state,
			0,
			IID_PPV_ARGS(&buffer->resource)));
#else
		D3D12MA::ALLOCATION_DESC allocation_desc = {};
		allocation_desc.HeapType = buffer->heap_type;

		D3D12MA::Allocation* allocation;
		checkResult(dx_context.memory_allocator->CreateResource(
			&allocation_desc,
			&buffer_desc,
			initial_state,
			0,
			&allocation,
			IID_PPV_ARGS(&buffer->resource)));

		buffer->allocation = allocation;
#endif

		buffer->gpu_virtual_address = buffer->resource->GetGPUVirtualAddress();


		uint32 num_descriptors = buffer->supports_srv + buffer->supports_uav + buffer->supports_clearing + buffer->raytracing;
		buffer->descriptor_allocation = dx_context.srv_uav_allocator.allocate(num_descriptors);

		uint32 index = 0;

		if (buffer->supports_srv)
		{
			buffer->default_srv = DXCPUDescriptorHandle(buffer->descriptor_allocation.cpu_at(index++)).create_buffer_srv(buffer);
		}

		if (buffer->supports_uav)
		{
			buffer->default_uav = DXCPUDescriptorHandle(buffer->descriptor_allocation.cpu_at(index++)).create_buffer_uav(buffer);
		}

		if (buffer->supports_clearing)
		{
			buffer->cpu_clear_uav = DXCPUDescriptorHandle(buffer->descriptor_allocation.cpu_at(index++)).create_buffer_uint_uav(buffer);

			buffer->shader_visible_descriptor_allocation = dx_context.srv_uav_allocator_shader_visible.allocate(1);
			DXCPUDescriptorHandle(buffer->shader_visible_descriptor_allocation.cpu_at(0)).create_buffer_uint_uav(buffer);
			buffer->gpu_clear_uav = buffer->shader_visible_descriptor_allocation.gpu_at(0);
		}

		if (buffer->raytracing)
		{
			buffer->raytracing_srv = DXCPUDescriptorHandle(buffer->descriptor_allocation.cpu_at(index++)).create_raytracing_acceleration_structure_srv(buffer);
		}

		ASSERT(index == num_descriptors);
	}

	BufferGrave::~BufferGrave()
	{
		if (resource)
		{
			dx_context.srv_uav_allocator.free(descriptor_allocation);
			dx_context.srv_uav_allocator_shader_visible.free(shader_visible_descriptor_allocation);
		}
	}

}