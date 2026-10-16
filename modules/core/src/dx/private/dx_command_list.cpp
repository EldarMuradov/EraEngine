// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "dx/dx_command_list.h"
#include "dx/dx_context.h"
		  
namespace era_engine
{
	DXCommandList::DXCommandList(D3D12_COMMAND_LIST_TYPE type)
	{
		this->type = type;
		checkResult(dx_context.device->CreateCommandAllocator(type, IID_PPV_ARGS(&command_allocator)));
		checkResult(dx_context.device->CreateCommandList(0, type, command_allocator.Get(), 0, IID_PPV_ARGS(&command_list)));

		dynamic_descriptor_heap.initialize();
	}

	void DXCommandList::barriers(CD3DX12_RESOURCE_BARRIER* barriers, uint32 num_barriers)
	{
		command_list->ResourceBarrier(num_barriers, barriers);
	}

	void DXCommandList::transition_barrier(const ref<DXTexture>& texture, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource)
	{
		transition_barrier(texture->resource, from, to, subresource);
	}

	void DXCommandList::transition_barrier(const ref<DXBuffer>& buffer, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource)
	{
		transition_barrier(buffer->resource, from, to, subresource);
	}

	void DXCommandList::transition_barrier(DXResource resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, uint32 subresource)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), from, to, subresource);
		command_list->ResourceBarrier(1, &barrier);
	}

	void DXCommandList::uav_barrier(const ref<DXTexture>& texture)
	{
		uav_barrier(texture->resource);
	}

	void DXCommandList::uav_barrier(const ref<DXBuffer>& buffer)
	{
		uav_barrier(buffer->resource);
	}

	void DXCommandList::uav_barrier(DXResource resource)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource.Get());
		command_list->ResourceBarrier(1, &barrier);
	}

	void DXCommandList::aliasing_barrier(const ref<DXTexture>& before, const ref<DXTexture>& after)
	{
		aliasing_barrier(before->resource, after->resource);
	}

	void DXCommandList::aliasing_barrier(const ref<DXBuffer>& before, const ref<DXBuffer>& after)
	{
		aliasing_barrier(before->resource, after->resource);
	}

	void DXCommandList::aliasing_barrier(DXResource before, DXResource after)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(before.Get(), after.Get());
		command_list->ResourceBarrier(1, &barrier);
	}

	void DXCommandList::assert_resource_state(const ref<DXTexture>& texture, D3D12_RESOURCE_STATES state, uint32 subresource)
	{
		assert_resource_state(texture->resource, state, subresource);
	}

	void DXCommandList::assert_resource_state(const ref<DXBuffer>& buffer, D3D12_RESOURCE_STATES state, uint32 subresource)
	{
		assert_resource_state(buffer->resource, state, subresource);
	}

	void DXCommandList::assert_resource_state(DXResource resource, D3D12_RESOURCE_STATES state, uint32 subresource)
	{
#ifdef _DEBUG
		ID3D12DebugCommandList* debugCL = 0;
		if (SUCCEEDED(command_list->QueryInterface(IID_PPV_ARGS(&debugCL))))
		{
			ASSERT(debugCL->AssertResourceState(resource.Get(), subresource, state));
		}
#endif
	}

	void DXCommandList::copy_resource(DXResource from, DXResource to)
	{
		command_list->CopyResource(to.Get(), from.Get());
	}

	void DXCommandList::copy_texture_region_to_buffer(const ref<DXTexture>& from, const ref<DXBuffer>& to, uint32 buffer_element_offset, uint32 x, uint32 y, uint32 width, uint32 height)
	{
		copy_texture_region_to_buffer(from->resource, from->width, from->format, to, buffer_element_offset, x, y, width, height);
	}

	void DXCommandList::copy_texture_region_to_buffer(const DXResource& from, uint32 from_width, DXGI_FORMAT format, const ref<DXBuffer>& to, uint32 buffer_element_offset, uint32 x, uint32 y, uint32 width, uint32 height)
	{
		uint32 num_pixels_to_copy = width * height;

		uint32 num_rows = bucketize(num_pixels_to_copy, from_width);
		uint32 dest_width = (num_rows == 1) ? to->element_count : from_width;

		D3D12_TEXTURE_COPY_LOCATION dest_location = {};
		dest_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dest_location.pResource = to->resource.Get();
		dest_location.SubresourceIndex = 0;
		dest_location.PlacedFootprint.Offset = 0;
		dest_location.PlacedFootprint.Footprint.Format = format;
		dest_location.PlacedFootprint.Footprint.Width = dest_width;
		dest_location.PlacedFootprint.Footprint.Height = num_rows;
		dest_location.PlacedFootprint.Footprint.Depth = 1;
		dest_location.PlacedFootprint.Footprint.RowPitch = align_to(dest_width * get_format_size(format), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

		D3D12_TEXTURE_COPY_LOCATION src_location = {};
		src_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src_location.SubresourceIndex = 0;
		src_location.pResource = from.Get();

		D3D12_BOX src_box = { x, y, 0, x + width, y + height, 1 };
		command_list->CopyTextureRegion(&dest_location, buffer_element_offset, 0, 0, &src_location, &src_box);
	}

	void DXCommandList::copy_texture_region_to_texture(const ref<DXTexture>& from, const ref<DXTexture>& to, uint32 source_x, uint32 source_y, uint32 dest_x, uint32 dest_y, uint32 width, uint32 height)
	{
		D3D12_TEXTURE_COPY_LOCATION dest_location = { to->resource.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
		D3D12_TEXTURE_COPY_LOCATION src_location = { from->resource.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };

		D3D12_BOX src_box = { source_x, source_y, 0, source_x + width, source_y + height, 1 };
		command_list->CopyTextureRegion(&dest_location, dest_x, dest_y, 0, &src_location, &src_box);
	}

	void DXCommandList::copy_buffer_region_to_buffer(const ref<DXBuffer>& from, const ref<DXBuffer>& to, uint32 from_element_offset, uint32 num_elements, uint32 to_element_offset)
	{
		command_list->CopyBufferRegion(to->resource.Get(), to_element_offset * to->element_size, from->resource.Get(), from_element_offset * from->element_size, num_elements * from->element_size);
	}

	void DXCommandList::copy_buffer_region_to_buffer_byte_offset(const ref<DXBuffer>& from, const ref<DXBuffer>& to, uint32 from_byte_offset, uint32 num_bytes, uint32 to_byte_offset)
	{
		command_list->CopyBufferRegion(to->resource.Get(), to_byte_offset, from->resource.Get(), from_byte_offset, num_bytes);
	}

	void DXCommandList::set_pipeline_state(DXPipelineState pipeline_state)
	{
		command_list->SetPipelineState(pipeline_state.Get());
	}

	void DXCommandList::set_pipeline_state(DXRaytracingPipelineState pipeline_state)
	{
		command_list->SetPipelineState1(pipeline_state.Get());
	}

	void DXCommandList::set_graphics_root_signature(const DXRootSignature& root_signature)
	{
		dynamic_descriptor_heap.parse_root_signature(root_signature);
		command_list->SetGraphicsRootSignature(root_signature.root_signature.Get());
	}

	void DXCommandList::set_graphics_32bit_constants(uint32 root_parameter_index, uint32 num_constants, const void* constants)
	{
		command_list->SetGraphicsRoot32BitConstants(root_parameter_index, num_constants, constants, 0);
	}

	void DXCommandList::set_compute_root_signature(const DXRootSignature& root_signature)
	{
		dynamic_descriptor_heap.parse_root_signature(root_signature);
		command_list->SetComputeRootSignature(root_signature.root_signature.Get());
	}

	void DXCommandList::set_compute_32bit_constants(uint32 root_parameter_index, uint32 num_constants, const void* constants)
	{
		command_list->SetComputeRoot32BitConstants(root_parameter_index, num_constants, constants, 0);
	}

	void DXCommandList::set_graphics_dynamic_constant_buffer(uint32 root_parameter_index, DXDynamicConstantBuffer address)
	{
		command_list->SetGraphicsRootConstantBufferView(root_parameter_index, address.gpu_ptr);
	}

	void DXCommandList::set_compute_dynamic_constant_buffer(uint32 root_parameter_index, DXDynamicConstantBuffer address)
	{
		command_list->SetComputeRootConstantBufferView(root_parameter_index, address.gpu_ptr);
	}

	void DXCommandList::set_root_graphics_uav(uint32 root_parameter_index, const ref<DXBuffer>& buffer)
	{
		command_list->SetGraphicsRootUnorderedAccessView(root_parameter_index, buffer->gpu_virtual_address);
	}

	void DXCommandList::set_root_graphics_uav(uint32 root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
	{
		command_list->SetGraphicsRootUnorderedAccessView(root_parameter_index, address);
	}

	void DXCommandList::set_root_compute_uav(uint32 root_parameter_index, const ref<DXBuffer>& buffer)
	{
		command_list->SetComputeRootUnorderedAccessView(root_parameter_index, buffer->gpu_virtual_address);
	}

	void DXCommandList::set_root_compute_uav(uint32 root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
	{
		command_list->SetComputeRootUnorderedAccessView(root_parameter_index, address);
	}

	void DXCommandList::set_root_graphics_srv(uint32 root_parameter_index, const ref<DXBuffer>& buffer)
	{
		command_list->SetGraphicsRootShaderResourceView(root_parameter_index, buffer->gpu_virtual_address);
	}

	void DXCommandList::set_root_graphics_srv(uint32 root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
	{
		command_list->SetGraphicsRootShaderResourceView(root_parameter_index, address);
	}

	void DXCommandList::set_root_compute_srv(uint32 root_parameter_index, const ref<DXBuffer>& buffer)
	{
		command_list->SetComputeRootShaderResourceView(root_parameter_index, buffer->gpu_virtual_address);
	}

	void DXCommandList::set_root_compute_srv(uint32 root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS address)
	{
		command_list->SetComputeRootShaderResourceView(root_parameter_index, address);
	}

	void DXCommandList::set_descriptor_heap_resource(uint32 root_parameter_index, uint32 offset, uint32 count, DXCPUDescriptorHandle handle)
	{
		dynamic_descriptor_heap.stage_descriptors(root_parameter_index, offset, count, handle);
	}

	void DXCommandList::set_descriptor_heap(DXDescriptorRange& descriptor_range)
	{
		descriptor_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = descriptor_range.descriptor_heap.Get();

		uint32 num_descriptor_heaps = 0;
		ID3D12DescriptorHeap* heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

		for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			ID3D12DescriptorHeap* heap = descriptor_heaps[i];
			if (heap)
			{
				heaps[num_descriptor_heaps++] = heap;
			}
		}

		command_list->SetDescriptorHeaps(num_descriptor_heaps, heaps);
	}

	void DXCommandList::set_descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE type, com<ID3D12DescriptorHeap> descriptor_heap)
	{
		descriptor_heaps[type] = descriptor_heap.Get();

		uint32 num_descriptor_heaps = 0;
		ID3D12DescriptorHeap* heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

		for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			ID3D12DescriptorHeap* heap = descriptor_heaps[i];
			if (heap)
			{
				heaps[num_descriptor_heaps++] = heap;
			}
		}

		command_list->SetDescriptorHeaps(num_descriptor_heaps, heaps);
	}

	void DXCommandList::reset_to_dynamic_descriptor_heap()
	{
		dynamic_descriptor_heap.set_current_descriptor_heap(this);
	}

	void DXCommandList::set_graphics_descriptor_table(uint32 root_parameter_index, CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
	{
		command_list->SetGraphicsRootDescriptorTable(root_parameter_index, handle);
	}

	void DXCommandList::set_compute_descriptor_table(uint32 root_parameter_index, CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
	{
		command_list->SetComputeRootDescriptorTable(root_parameter_index, handle);
	}

	void DXCommandList::clear_uav(const ref<DXBuffer>& buffer, float val)
	{
		clear_uav(buffer->resource, buffer->cpu_clear_uav, buffer->gpu_clear_uav, val);
	}

	void DXCommandList::clear_uav(const ref<DXBuffer>& buffer, uint32 val)
	{
		clear_uav(buffer->resource, buffer->cpu_clear_uav, buffer->gpu_clear_uav, val);
	}

	void DXCommandList::clear_uav(DXResource resource, DXCPUDescriptorHandle cpu_handle, DXGPUDescriptorHandle gpu_handle, float val)
	{
		float vals[] = { val, val, val, val };
		command_list->ClearUnorderedAccessViewFloat(gpu_handle, cpu_handle, resource.Get(), vals, 0, 0);
	}

	void DXCommandList::clear_uav(DXResource resource, DXCPUDescriptorHandle cpu_handle, DXGPUDescriptorHandle gpu_handle, uint32 val)
	{
		uint32 vals[] = { val, val, val, val };
		command_list->ClearUnorderedAccessViewUint(gpu_handle, cpu_handle, resource.Get(), vals, 0, 0);
	}

	void DXCommandList::set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY topology)
	{
		command_list->IASetPrimitiveTopology(topology);
	}

	void DXCommandList::set_vertex_buffer(uint32 slot, const ref<DXVertexBuffer>& buffer)
	{
		command_list->IASetVertexBuffers(slot, 1, &buffer->view);
	}

	void DXCommandList::set_vertex_buffer(uint32 slot, const DXDynamicVertexBuffer& buffer)
	{
		command_list->IASetVertexBuffers(slot, 1, &buffer.view);
	}

	void DXCommandList::set_vertex_buffer(uint32 slot, const D3D12_VERTEX_BUFFER_VIEW& buffer)
	{
		command_list->IASetVertexBuffers(slot, 1, &buffer);
	}

	void DXCommandList::set_index_buffer(const ref<DXIndexBuffer>& buffer)
	{
		command_list->IASetIndexBuffer(&buffer->view);
	}

	void DXCommandList::set_index_buffer(const DXDynamicIndexBuffer& buffer)
	{
		command_list->IASetIndexBuffer(&buffer.view);
	}

	void DXCommandList::set_index_buffer(const D3D12_INDEX_BUFFER_VIEW& buffer)
	{
		command_list->IASetIndexBuffer(&buffer);
	}

	void DXCommandList::set_viewport(const D3D12_VIEWPORT& viewport)
	{
		command_list->RSSetViewports(1, &viewport);
	}

	void DXCommandList::set_viewport(float x, float y, float width, float height, float min_depth, float max_depth)
	{
		D3D12_VIEWPORT viewport = { x, y, width, height, min_depth, max_depth };
		set_viewport(viewport);
	}

	void DXCommandList::set_scissor(const D3D12_RECT& scissor)
	{
		command_list->RSSetScissorRects(1, &scissor);
	}

	void DXCommandList::set_render_target(const DXRTVDescriptorHandle* rtvs, uint32 num_rtvs, const DXDSVDescriptorHandle* dsv)
	{
		command_list->OMSetRenderTargets(num_rtvs, (D3D12_CPU_DESCRIPTOR_HANDLE*)rtvs, FALSE, (D3D12_CPU_DESCRIPTOR_HANDLE*)dsv);
	}

	void DXCommandList::set_render_target(const DXRenderTarget& render_target)
	{
		set_render_target(render_target.rtv, render_target.num_attachments, render_target.dsv ? &render_target.dsv : 0);
	}

	static void get_d3d12_rects(D3D12_RECT* d3rects, const ClearRect* rects, uint32 num_rects)
	{
		for (uint32 i = 0; i < num_rects; ++i)
		{
			d3rects[i] = { (LONG)rects[i].x, (LONG)rects[i].y, (LONG)rects[i].x + (LONG)rects[i].width, (LONG)rects[i].y + (LONG)rects[i].height };
		}
	}

	void DXCommandList::clear_rtv(DXRTVDescriptorHandle rtv, float r, float g, float b, float a, const ClearRect* rects, uint32 num_rects)
	{
		float clear_color[] = { r, g, b, a };
		clear_rtv(rtv, clear_color, rects, num_rects);
	}

	void DXCommandList::clear_rtv(DXRTVDescriptorHandle rtv, const float* clear_color, const ClearRect* rects, uint32 num_rects)
	{
		D3D12_RECT* d3rects = 0;
		if (num_rects)
		{
			d3rects = (D3D12_RECT*)alloca(sizeof(D3D12_RECT) * num_rects);
			get_d3d12_rects(d3rects, rects, num_rects);
		}

		command_list->ClearRenderTargetView(rtv, clear_color, num_rects, d3rects);
	}

	void DXCommandList::clear_rtv(const ref<DXTexture>& texture, float r, float g, float b, float a, const ClearRect* rects, uint32 num_rects)
	{
		clear_rtv(texture->default_rtv, r, g, b, a, rects, num_rects);
	}

	void DXCommandList::clear_rtv(const ref<DXTexture>& texture, const float* clear_color, const ClearRect* rects, uint32 num_rects)
	{
		clear_rtv(texture->default_rtv, clear_color, rects, num_rects);
	}

	void DXCommandList::clear_rtv(const DXRenderTarget& render_target, uint32 attachment, const float* clear_color, const ClearRect* rects, uint32 num_rects)
	{
		clear_rtv(render_target.rtv[attachment], clear_color, rects, num_rects);
	}

	void DXCommandList::clear_rtv(const DXRenderTarget& render_target, uint32 attachment, float r, float g, float b, float a, const ClearRect* rects, uint32 num_rects)
	{
		clear_rtv(render_target.rtv[attachment], r, g, b, a, rects, num_rects);
	}

	void DXCommandList::clear_depth(DXDSVDescriptorHandle dsv, float depth, const ClearRect* rects, uint32 num_rects)
	{
		D3D12_RECT* d3rects = 0;
		if (num_rects)
		{
			d3rects = (D3D12_RECT*)alloca(sizeof(D3D12_RECT) * num_rects);
			get_d3d12_rects(d3rects, rects, num_rects);
		}

		command_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, num_rects, d3rects);
	}

	void DXCommandList::clear_depth(const ref<DXTexture>& texture, float depth, const ClearRect* rects, uint32 num_rects)
	{
		clear_depth(texture->default_dsv, depth, rects, num_rects);
	}

	void DXCommandList::clear_depth(const DXRenderTarget& render_target, float depth, const ClearRect* rects, uint32 num_rects)
	{
		clear_depth(render_target.dsv, depth, rects, num_rects);
	}

	void DXCommandList::clear_stencil(DXDSVDescriptorHandle dsv, uint32 stencil, const ClearRect* rects, uint32 num_rects)
	{
		D3D12_RECT* d3rects = 0;
		if (num_rects)
		{
			d3rects = (D3D12_RECT*)alloca(sizeof(D3D12_RECT) * num_rects);
			get_d3d12_rects(d3rects, rects, num_rects);
		}

		command_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_STENCIL, 0.f, stencil, num_rects, d3rects);
	}

	void DXCommandList::clear_stencil(const ref<DXTexture>& texture, uint32 stencil, const ClearRect* rects, uint32 num_rects)
	{
		clear_stencil(texture->default_dsv, stencil, rects, num_rects);
	}

	void DXCommandList::clear_stencil(const DXRenderTarget& render_target, uint32 stencil, const ClearRect* rects, uint32 num_rects)
	{
		clear_stencil(render_target.dsv, stencil, rects, num_rects);
	}

	void DXCommandList::clear_depth_and_stencil(DXDSVDescriptorHandle dsv, float depth, uint32 stencil, const ClearRect* rects, uint32 num_rects)
	{
		D3D12_RECT* d3rects = 0;
		if (num_rects)
		{
			d3rects = (D3D12_RECT*)alloca(sizeof(D3D12_RECT) * num_rects);
			get_d3d12_rects(d3rects, rects, num_rects);
		}

		command_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, num_rects, d3rects);
	}

	void DXCommandList::clear_depth_and_stencil(const ref<DXTexture>& texture, float depth, uint32 stencil, const ClearRect* rects, uint32 num_rects)
	{
		clear_depth_and_stencil(texture->default_dsv, depth, stencil, rects, num_rects);
	}

	void DXCommandList::clear_depth_and_stencil(const DXRenderTarget& render_target, float depth, uint32 stencil, const ClearRect* rects, uint32 num_rects)
	{
		clear_depth_and_stencil(render_target.dsv, depth, stencil, rects, num_rects);
	}

	void DXCommandList::set_stencil_reference(uint32 stencil_reference)
	{
		command_list->OMSetStencilRef(stencil_reference);
	}

	void DXCommandList::set_blend_factor(float blend_r, float blend_g, float blend_b, float blend_a)
	{
		const float blend_factor[] = { blend_r, blend_g, blend_b, blend_a };
		command_list->OMSetBlendFactor(blend_factor);
	}

	void DXCommandList::draw(uint32 vertex_count, uint32 instance_count, uint32 start_vertex, uint32 start_instance)
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_draw(this);
		command_list->DrawInstanced(vertex_count, instance_count, start_vertex, start_instance);
	}

	void DXCommandList::draw_indexed(uint32 index_count, uint32 instance_count, uint32 start_index, int32 base_vertex, uint32 start_instance)
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_draw(this);
		command_list->DrawIndexedInstanced(index_count, instance_count, start_index, base_vertex, start_instance);
	}

	void DXCommandList::draw_indirect(DXCommandSignature command_signature, uint32 num_commands, const DXResource& command_buffer, uint32 command_buffer_offset)
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_draw(this);
		command_list->ExecuteIndirect(
			command_signature.Get(),
			num_commands,
			command_buffer.Get(),
			command_buffer_offset,
			0,
			0);
	}

	void DXCommandList::draw_indirect(DXCommandSignature command_signature, uint32 max_num_commands, const DXResource& num_draws_buffer, const DXResource& command_buffer, uint32 command_buffer_offset)
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_draw(this);
		command_list->ExecuteIndirect(
			command_signature.Get(),
			max_num_commands,
			command_buffer.Get(),
			command_buffer_offset,
			num_draws_buffer.Get(),
			0);
	}

	void DXCommandList::draw_indirect(DXCommandSignature command_signature, uint32 num_commands, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset)
	{
		draw_indirect(command_signature, num_commands, command_buffer->resource, command_buffer_offset);
	}

	void DXCommandList::draw_indirect(DXCommandSignature command_signature, uint32 max_num_commands, const ref<DXBuffer>& num_draws_buffer, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset)
	{
		draw_indirect(command_signature, max_num_commands, num_draws_buffer->resource, command_buffer->resource, command_buffer_offset);
	}

	void DXCommandList::draw_fullscreen_triangle()
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_draw(this);
		set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		draw(3, 1, 0, 0);
	}

	void DXCommandList::draw_cube_triangle_strip()
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_draw(this);
		set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		draw(14, 1, 0, 0);
	}

	void DXCommandList::dispatch(uint32 num_groups_x, uint32 num_groups_y, uint32 num_groups_z)
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_dispatch(this);
		command_list->Dispatch(num_groups_x, num_groups_y, num_groups_z);
	}

	void DXCommandList::dispatch_indirect(DXCommandSignature command_signature, uint32 num_commands, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset)
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_dispatch(this);
		command_list->ExecuteIndirect(
			command_signature.Get(),
			num_commands,
			command_buffer->resource.Get(),
			command_buffer_offset,
			0,
			0);
	}

	void DXCommandList::dispatch_indirect(DXCommandSignature command_signature, uint32 max_num_commands, const ref<DXBuffer>& num_dispatches_buffer, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset)
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_dispatch(this);
		command_list->ExecuteIndirect(
			command_signature.Get(),
			max_num_commands,
			command_buffer->resource.Get(),
			command_buffer_offset,
			num_dispatches_buffer->resource.Get(),
			0);
	}

	void DXCommandList::dispatch_indirect(uint32 num_commands, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset)
	{
		dispatch_indirect(dx_context.default_dispatch_command_signature, num_commands, command_buffer, command_buffer_offset);
	}

	void DXCommandList::dispatch_indirect(uint32 max_num_commands, const ref<DXBuffer>& num_dispatches_buffer, const ref<DXBuffer>& command_buffer, uint32 command_buffer_offset)
	{
		dispatch_indirect(dx_context.default_dispatch_command_signature, max_num_commands, num_dispatches_buffer, command_buffer, command_buffer_offset);
	}

	void DXCommandList::dispatch_mesh(uint32 num_groups_x, uint32 num_groups_y, uint32 num_groups_z)
	{
#ifdef SDK_SUPPORTS_MESH_SHADERS
		dynamic_descriptor_heap.commit_staged_descriptors_for_draw(this);
		command_list->DispatchMesh(num_groups_x, num_groups_y, num_groups_z);
#else
		ASSERT(!"Mesh shaders are not supported with your Windows SDK version.");
#endif
	}

	void DXCommandList::raytrace(D3D12_DISPATCH_RAYS_DESC& raytrace_desc)
	{
		dynamic_descriptor_heap.commit_staged_descriptors_for_dispatch(this);
		command_list->DispatchRays(&raytrace_desc);
	}

	void DXCommandList::query_timestamp(uint32 index)
	{
		command_list->EndQuery(time_stamp_query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index);
	}

	void DXCommandList::reset()
	{
		command_allocator->Reset();
		checkResult(command_list->Reset(command_allocator.Get(), 0));

		for (uint32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			descriptor_heaps[i] = 0;
		}

		dynamic_descriptor_heap.reset();
	}

}