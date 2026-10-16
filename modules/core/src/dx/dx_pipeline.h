// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "dx/dx.h"
#include "dx/dx_context.h"

#define CREATE_GRAPHICS_PIPELINE DXGraphicsPipelineGenerator()
#define UNBOUNDED_DESCRIPTOR_RANGE -1

namespace era_engine
{
	struct DXRootSignature
	{
		com<ID3D12RootSignature> root_signature;
		uint32* descriptor_table_sizes;
		uint32 num_descriptor_tables;
		uint32 table_root_parameter_mask;
		uint32 total_num_parameters;
	};

	NODISCARD DXRootSignature create_root_signature(DXBlob root_signature_blob);
	NODISCARD DXRootSignature create_root_signature(const wchar* path);
	NODISCARD DXRootSignature create_root_signature(const D3D12_ROOT_SIGNATURE_DESC1& desc);
	NODISCARD DXRootSignature create_root_signature(CD3DX12_ROOT_PARAMETER1* root_parameters, uint32 num_root_parameters, CD3DX12_STATIC_SAMPLER_DESC* samplers, uint32 num_samplers,
		D3D12_ROOT_SIGNATURE_FLAGS flags);
	NODISCARD DXRootSignature create_root_signature(const D3D12_ROOT_SIGNATURE_DESC& desc);
	NODISCARD DXRootSignature create_root_signature(CD3DX12_ROOT_PARAMETER* root_parameters, uint32 num_root_parameters, CD3DX12_STATIC_SAMPLER_DESC* samplers, uint32 num_samplers,
		D3D12_ROOT_SIGNATURE_FLAGS flags);
	NODISCARD DXCommandSignature create_command_signature(DXRootSignature root_signature, const D3D12_COMMAND_SIGNATURE_DESC& command_signature_desc);
	NODISCARD DXRootSignature create_root_signature(D3D12_ROOT_SIGNATURE_FLAGS flags);
	void free_root_signature(DXRootSignature& rs);

	DXCommandSignature create_command_signature(DXRootSignature root_signature, D3D12_INDIRECT_ARGUMENT_DESC* argument_descs, uint32 num_argument_descs, uint32 command_structure_size);

	struct DXGraphicsPipelineGenerator
	{
		operator const D3D12_GRAPHICS_PIPELINE_STATE_DESC& () const
		{
			return desc;
		}

		DXGraphicsPipelineGenerator()
		{
			desc = {};
			desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			desc.RasterizerState.FrontCounterClockwise = true;
			desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			desc.SampleDesc = { 1, 0 };
			desc.SampleMask = 0xFFFFFFFF;
		}

		DXGraphicsPipelineGenerator& root_signature(const DXRootSignature& root_signature)
		{
			desc.pRootSignature = root_signature.root_signature.Get();
			return *this;
		}

		DXGraphicsPipelineGenerator& vs(DXBlob shader)
		{
			desc.VS = CD3DX12_SHADER_BYTECODE(shader.Get());
			return *this;
		}

		DXGraphicsPipelineGenerator& ps(DXBlob shader)
		{
			desc.PS = CD3DX12_SHADER_BYTECODE(shader.Get());
			return *this;
		}

		DXGraphicsPipelineGenerator& gs(DXBlob shader)
		{
			desc.GS = CD3DX12_SHADER_BYTECODE(shader.Get());
			return *this;
		}

		DXGraphicsPipelineGenerator& ds(DXBlob shader)
		{
			desc.DS = CD3DX12_SHADER_BYTECODE(shader.Get());
			return *this;
		}

		DXGraphicsPipelineGenerator& hs(DXBlob shader)
		{
			desc.HS = CD3DX12_SHADER_BYTECODE(shader.Get());
			return *this;
		}

		DXGraphicsPipelineGenerator& alpha_to_coverage(bool enable)
		{
			desc.BlendState.AlphaToCoverageEnable = enable;
			return *this;
		}

		DXGraphicsPipelineGenerator& independent_render_target_blending(bool enable)
		{
			desc.BlendState.IndependentBlendEnable = enable;
			return *this;
		}

		DXGraphicsPipelineGenerator& blend_state(uint32 render_target_index, D3D12_BLEND src_blend, D3D12_BLEND dest_blend, D3D12_BLEND_OP op)
		{
			ASSERT(!desc.BlendState.RenderTarget[render_target_index].LogicOpEnable);
			desc.BlendState.RenderTarget[render_target_index].BlendEnable = true;
			desc.BlendState.RenderTarget[render_target_index].SrcBlend = src_blend;
			desc.BlendState.RenderTarget[render_target_index].DestBlend = dest_blend;
			desc.BlendState.RenderTarget[render_target_index].BlendOp = op;
			return *this;
		}

		DXGraphicsPipelineGenerator& alpha_blending(uint32 render_target_index)
		{
			return blend_state(render_target_index, D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD);
		}

		DXGraphicsPipelineGenerator& additive_blending(uint32 render_target_index)
		{
			return blend_state(render_target_index, D3D12_BLEND_ONE, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD);
		}

		DXGraphicsPipelineGenerator& wireframe()
		{
			desc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
			return *this;
		}

		DXGraphicsPipelineGenerator& cull_front_faces()
		{
			desc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
			return *this;
		}

		DXGraphicsPipelineGenerator& culling_off()
		{
			desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			return *this;
		}

		DXGraphicsPipelineGenerator& rasterize_clockwise()
		{
			desc.RasterizerState.FrontCounterClockwise = false;
			return *this;
		}

		DXGraphicsPipelineGenerator& depth_bias(int bias = D3D12_DEFAULT_DEPTH_BIAS, float bias_clamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP, float slope_scaled_bias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS)
		{
			desc.RasterizerState.DepthBias = bias;
			desc.RasterizerState.DepthBiasClamp = bias_clamp;
			desc.RasterizerState.SlopeScaledDepthBias = slope_scaled_bias;
			return *this;
		}

		DXGraphicsPipelineGenerator& antialiasing(bool multisampling = false, bool antialiasedlines = false, uint32 forced_sample_count = 0)
		{
			desc.RasterizerState.MultisampleEnable = multisampling;
			desc.RasterizerState.AntialiasedLineEnable = antialiasedlines;
			desc.RasterizerState.ForcedSampleCount = forced_sample_count;
			return *this;
		}

		DXGraphicsPipelineGenerator& rasterize_convervative()
		{
			desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
			return *this;
		}

		DXGraphicsPipelineGenerator& depth_settings(bool depth_test = true, bool depth_write = true, D3D12_COMPARISON_FUNC func = D3D12_COMPARISON_FUNC_LESS)
		{
			desc.DepthStencilState.DepthEnable = depth_test;
			desc.DepthStencilState.DepthWriteMask = depth_write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
			desc.DepthStencilState.DepthFunc = func;
			return *this;
		}

		DXGraphicsPipelineGenerator& stencil_settings(
			D3D12_COMPARISON_FUNC func = D3D12_COMPARISON_FUNC_ALWAYS,
			D3D12_STENCIL_OP on_pass = D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP on_stencil_pass_and_depth_fail = D3D12_STENCIL_OP_KEEP,
			D3D12_STENCIL_OP on_fail = D3D12_STENCIL_OP_KEEP,
			uint8 read_mask = D3D12_DEFAULT_STENCIL_READ_MASK, uint8 write_mask = D3D12_DEFAULT_STENCIL_WRITE_MASK)
		{
			desc.DepthStencilState.StencilEnable = true;
			desc.DepthStencilState.StencilReadMask = read_mask;
			desc.DepthStencilState.StencilWriteMask = write_mask;

			desc.DepthStencilState.FrontFace.StencilFailOp = on_fail;
			desc.DepthStencilState.FrontFace.StencilDepthFailOp = on_stencil_pass_and_depth_fail;
			desc.DepthStencilState.FrontFace.StencilPassOp = on_pass;
			desc.DepthStencilState.FrontFace.StencilFunc = func;

			desc.DepthStencilState.BackFace.StencilFailOp = on_fail;
			desc.DepthStencilState.BackFace.StencilDepthFailOp = on_stencil_pass_and_depth_fail;
			desc.DepthStencilState.BackFace.StencilPassOp = on_pass;
			desc.DepthStencilState.BackFace.StencilFunc = func;
			return *this;
		}

		template <uint32 numElements>
		DXGraphicsPipelineGenerator& input_layout(D3D12_INPUT_ELEMENT_DESC(&elements)[numElements])
		{
			desc.InputLayout = { elements, numElements };
			return *this;
		}

		DXGraphicsPipelineGenerator& input_layout(D3D12_INPUT_ELEMENT_DESC* elements, uint32 num_elements)
		{
			desc.InputLayout = { elements, num_elements };
			return *this;
		}

		DXGraphicsPipelineGenerator& primitive_topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
		{
			desc.PrimitiveTopologyType = type;
			return *this;
		}

		DXGraphicsPipelineGenerator& render_targets(DXGI_FORMAT rt_format, DXGI_FORMAT dsv_format = DXGI_FORMAT_UNKNOWN)
		{
			return render_targets(&rt_format, 1, dsv_format);
		}

		DXGraphicsPipelineGenerator& render_targets(const DXGI_FORMAT* rt_formats, uint32 num_render_targets, DXGI_FORMAT dsv_format = DXGI_FORMAT_UNKNOWN)
		{
			memcpy(desc.RTVFormats, rt_formats, sizeof(DXGI_FORMAT) * num_render_targets);
			desc.NumRenderTargets = num_render_targets;
			desc.DSVFormat = dsv_format;
			return *this;
		}

		DXGraphicsPipelineGenerator& render_targets(const D3D12_RT_FORMAT_ARRAY& rt_formats, DXGI_FORMAT dsv_format = DXGI_FORMAT_UNKNOWN)
		{
			return render_targets(rt_formats.RTFormats, rt_formats.NumRenderTargets, dsv_format);
		}

		DXGraphicsPipelineGenerator& multisampling(uint32 count = 1, uint32 quality = 0)
		{
			desc.SampleDesc = { count, quality };
			return *this;
		}

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	};

	struct root_descriptor_table : CD3DX12_ROOT_PARAMETER
	{
		root_descriptor_table(uint32 numDescriptorRanges,
			const D3D12_DESCRIPTOR_RANGE* descriptorRanges,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsDescriptorTable(numDescriptorRanges, descriptorRanges, visibility);
		}
	};

	template <typename T>
	struct root_constants : CD3DX12_ROOT_PARAMETER
	{
		root_constants(uint32 shaderRegister,
			uint32 space = 0,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsConstants(sizeof(T) / 4, shaderRegister, space, visibility);
		}
	};

	struct root_cbv : CD3DX12_ROOT_PARAMETER
	{
		root_cbv(uint32 shaderRegister,
			uint32 space = 0,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsConstantBufferView(shaderRegister, space, visibility);
		}
	};

	struct root_srv : CD3DX12_ROOT_PARAMETER
	{
		root_srv(uint32 shaderRegister,
			uint32 space = 0,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsShaderResourceView(shaderRegister, space, visibility);
		}
	};

	struct root_uav : CD3DX12_ROOT_PARAMETER
	{
		root_uav(uint32 shaderRegister,
			uint32 space = 0,
			D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			InitAsUnorderedAccessView(shaderRegister, space, visibility);
		}
	};

	struct indirect_vertex_buffer : D3D12_INDIRECT_ARGUMENT_DESC
	{
		indirect_vertex_buffer(uint32 slot)
		{
			Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
			VertexBuffer.Slot = slot;
		}
	};

	struct indirect_index_buffer : D3D12_INDIRECT_ARGUMENT_DESC
	{
		indirect_index_buffer()
		{
			Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
		}
	};

	template <typename T>
	struct indirect_root_constants : D3D12_INDIRECT_ARGUMENT_DESC
	{
		indirect_root_constants(uint32 rootParameterIndex, uint32 destOffsetIn32BitValues = 0)
		{
			Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			Constant.Num32BitValuesToSet = sizeof(T) / 4;
			Constant.RootParameterIndex = rootParameterIndex;
			Constant.DestOffsetIn32BitValues = destOffsetIn32BitValues;
		}
	};

	struct indirect_cbv : D3D12_INDIRECT_ARGUMENT_DESC
	{
		indirect_cbv(uint32 rootParameterIndex)
		{
			Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
			ConstantBufferView.RootParameterIndex = rootParameterIndex;
		}
	};

	struct indirect_srv : D3D12_INDIRECT_ARGUMENT_DESC
	{
		indirect_srv(uint32 rootParameterIndex)
		{
			Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
			ConstantBufferView.RootParameterIndex = rootParameterIndex;
		}
	};

	struct indirect_uav : D3D12_INDIRECT_ARGUMENT_DESC
	{
		indirect_uav(uint32 rootParameterIndex)
		{
			Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
			ConstantBufferView.RootParameterIndex = rootParameterIndex;
		}
	};

	struct indirect_draw : D3D12_INDIRECT_ARGUMENT_DESC
	{
		indirect_draw()
		{
			Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
		}
	};

	struct indirect_draw_indexed : D3D12_INDIRECT_ARGUMENT_DESC
	{
		indirect_draw_indexed()
		{
			Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
		}
	};

	struct indirect_dispatch : D3D12_INDIRECT_ARGUMENT_DESC
	{
		indirect_dispatch()
		{
			Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
		}
	};

	struct DXPipelineStreamBase
	{
		virtual void set_vertex_shader(DXBlob blob) {}
		virtual void set_pixel_shader(DXBlob blob) {}
		virtual void set_domain_shader(DXBlob blob) {}
		virtual void set_hull_shader(DXBlob blob) {}
		virtual void set_geometry_shader(DXBlob blob) {}
		virtual void set_mesh_shader(DXBlob blob) {}
		virtual void set_amplification_shader(DXBlob blob) {}

		virtual void set_root_signature(DXRootSignature rs) = 0;
	};

	struct DXPipeline
	{
		DXPipelineState* pipeline;
		DXRootSignature* root_signature;
	};

	union GraphicsPipelineFiles
	{
		struct
		{
			const char* vs;
			const char* ps;
			const char* ds;
			const char* hs;
			const char* gs;
			const char* ms;
			const char* as;
		};
		const char* shaders[7] = {};
	};

	enum RSFile
	{
		RS_IN_VERTEX_SHADER,
		RS_IN_PIXEL_SHADER,
		RS_IN_DOMAIN_SHADER,
		RS_IN_HULL_SHADER,
		RS_IN_GEOMETRY_SHADER,
		RS_IN_MESH_SHADER,
		RS_IN_AMPLIFICATION_SHADER,
	};

	DXPipeline create_reloadable_pipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const GraphicsPipelineFiles& files, DXRootSignature user_root_signature, bool load_immediately = false);
	DXPipeline create_reloadable_pipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const GraphicsPipelineFiles& files, RSFile root_signature_file = RS_IN_PIXEL_SHADER, bool load_immediately = false);

	DXPipeline create_reloadable_pipeline(const char* cs_file, DXRootSignature user_root_signature, bool load_immediately = false);
	DXPipeline create_reloadable_pipeline(const char* cs_file, bool load_immediately = false);

	DXPipeline create_reloadable_pipeline(const D3D12_PIPELINE_STATE_STREAM_DESC& desc, DXPipelineStreamBase* stream, const GraphicsPipelineFiles& files, DXRootSignature user_root_signature, bool load_immediately);
	DXPipeline create_reloadable_pipeline(const D3D12_PIPELINE_STATE_STREAM_DESC& desc, DXPipelineStreamBase* stream, const GraphicsPipelineFiles& files, RSFile root_signature_file, bool load_immediately);

	template <typename stream_t>
	inline NODISCARD DXPipeline create_reloadable_pipeline_from_stream(const stream_t& stream, const GraphicsPipelineFiles& files, DXRootSignature user_root_signature)
	{
		static_assert(std::is_base_of_v<DXPipelineStreamBase, stream_t>, "Stream must inherit from DXPipelineStreamBase.");

		stream_t* stream_copy = new stream_t(stream); // Dynamically allocated for permanent storage.
		D3D12_PIPELINE_STATE_STREAM_DESC desc = {
			sizeof(stream_t) - 8, (uint8*)stream_copy + 8 // Offset for vTable. This seems very broken. TODO: Verify that this always works.
		};

		return create_reloadable_pipeline(desc, stream_copy, files, user_root_signature, false);
	}

	template <typename stream_t>
	inline NODISCARD DXPipeline create_reloadable_pipeline_from_stream(const stream_t& stream, const GraphicsPipelineFiles& files, RSFile root_signature_file = RS_IN_PIXEL_SHADER)
	{
		static_assert(std::is_base_of_v<DXPipelineStreamBase, stream_t>, "Stream must inherit from DXPipelineStreamBase.");

		stream_t* stream_copy = new stream_t(stream); // Dynamically allocated for permanent storage.
		D3D12_PIPELINE_STATE_STREAM_DESC desc = {
			sizeof(stream_t) - 8, (uint8*)stream_copy + 8 // Offset for vTable. This seems very broken. TODO: Verify that this always works.
		};

		return create_reloadable_pipeline(desc, stream_copy, files, root_signature_file, false);
	}

	void create_all_pending_reloadable_pipelines();

	void check_for_changed_pipelines();
}