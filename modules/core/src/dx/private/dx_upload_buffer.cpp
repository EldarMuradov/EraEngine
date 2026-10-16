// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "dx/dx_upload_buffer.h"
#include "dx/dx_context.h"

#include "core/memory.h"

namespace era_engine
{
	NODISCARD DXPage* DXPagePool::allocate_new_page()
	{
		mutex.lock();
		DXPage* result = arena.allocate<DXPage>(1, true);
		mutex.unlock();

		auto desc = CD3DX12_RESOURCE_DESC::Buffer(page_size);
		CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
		checkResult(dx_context.device->CreateCommittedResource(
			&props,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&result->buffer)
		));

		result->gpu_base_ptr = result->buffer->GetGPUVirtualAddress();
		result->buffer->Map(0, 0, (void**)&result->cpu_base_ptr);
		result->page_size = page_size;

		return result;
	}

	NODISCARD DXPage* DXPagePool::get_free_page()
	{
		mutex.lock();
		DXPage* result = free_pages;
		if (result)
		{
			free_pages = result->next;
		}
		mutex.unlock();

		if (!result)
		{
			result = allocate_new_page();
		}

		result->current_offset = 0;

		return result;
	}

	void DXPagePool::return_page(DXPage* page)
	{
		Lock lock{ mutex };
		page->next = used_pages;
		used_pages = page;
		if (!last_used_page)
		{
			last_used_page = page;
		}
	}

	void DXPagePool::reset()
	{
		if (last_used_page)
		{
			last_used_page->next = free_pages;
		}
		free_pages = used_pages;
		used_pages = 0;
		last_used_page = 0;
	}

	NODISCARD DXAllocation DXUploadBuffer::allocate(uint64 size, uint64 alignment)
	{
		ASSERT(size <= page_pool->page_size);

		uint64 aligned_offset = current_page ? align_to(current_page->current_offset, alignment) : 0;

		DXPage* page = current_page;
		if (!page || aligned_offset + size > page->page_size)
		{
			page = page_pool->get_free_page();
			aligned_offset = 0;

			if (current_page)
			{
				page_pool->return_page(current_page);
			}
			current_page = page;
		}

		DXAllocation result;
		result.cpu_ptr = page->cpu_base_ptr + aligned_offset;
		result.gpu_ptr = page->gpu_base_ptr + aligned_offset;
		result.resource = page->buffer;
		result.offset_in_resource = (uint32)aligned_offset;

		page->current_offset = aligned_offset + size;

		return result;
	}

	void DXUploadBuffer::reset()
	{
		if (current_page && page_pool)
		{
			page_pool->return_page(current_page);
		}
		current_page = 0;
	}

	void DXPagePool::initialize(uint32 size_in_bytes)
	{
		page_size = size_in_bytes;
		arena.initialize(0, sizeof(DXPage) * 512);
	}
}