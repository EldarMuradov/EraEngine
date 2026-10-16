// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "dx/dx_pipeline.h"

#include "core/string.h"
#include "core/file_system.h"
#include "core/log.h"

namespace era_engine
{
	static void handle_pipeline_changes(const FileSystemEvent& e);
	static void load_root_signature(struct ReloadableRootSignature& r);
	static void load_pipeline(struct ReloadablePipelineState& p);

	struct ShaderFile
	{
		DXBlob blob;
		std::set<struct ReloadablePipelineState*> used_by_pipelines;

		struct ReloadableRootSignature* root_signature;
	};

	enum DescType
	{
		DESC_TYPE_STRUCT,
		DESC_TYPE_STREAM,
	};

	union GraphicsPipelineDesc
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC struct_desc;
		D3D12_PIPELINE_STATE_STREAM_DESC stream_desc;

		GraphicsPipelineDesc() = default;
	};

	enum PipelineType
	{
		PIPELINE_TYPE_GRAPHICS,
		PIPELINE_TYPE_COMPUTE,
	};

	struct ReloadablePipelineState
	{
		PipelineType pipeline_type;
		DescType description_type;

		union
		{
			struct
			{
				D3D12_GRAPHICS_PIPELINE_STATE_DESC graphics_desc;

				D3D12_PIPELINE_STATE_STREAM_DESC stream_desc;
				DXPipelineStreamBase* stream;

				GraphicsPipelineFiles graphics_files;
			};

			struct
			{
				D3D12_COMPUTE_PIPELINE_STATE_DESC compute_desc;
				const char* compute_file;
			};
		};

		DXPipelineState pipeline;
		DXRootSignature* root_signature;

		D3D12_INPUT_ELEMENT_DESC input_layout[16];

		ReloadablePipelineState() {}

		void initialize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const GraphicsPipelineFiles& files, DXRootSignature* root_signature)
		{
			this->pipeline_type = PIPELINE_TYPE_GRAPHICS;
			this->description_type = DESC_TYPE_STRUCT;
			this->graphics_desc = desc;
			this->graphics_files = files;
			this->root_signature = root_signature;

			ASSERT(desc.InputLayout.NumElements <= arraysize(input_layout));

			memcpy(input_layout, desc.InputLayout.pInputElementDescs, sizeof(D3D12_INPUT_ELEMENT_DESC) * desc.InputLayout.NumElements);
			this->graphics_desc.InputLayout.pInputElementDescs = input_layout;

			if (desc.InputLayout.NumElements == 0)
			{
				this->graphics_desc.InputLayout.pInputElementDescs = nullptr;
			}
		}

		void initialize(const D3D12_PIPELINE_STATE_STREAM_DESC& desc, DXPipelineStreamBase* stream, const GraphicsPipelineFiles& files, DXRootSignature* root_signature)
		{
			this->pipeline_type = PIPELINE_TYPE_GRAPHICS;
			this->description_type = DESC_TYPE_STREAM;
			this->stream_desc = desc;
			this->stream = stream;
			this->graphics_files = files;
			this->root_signature = root_signature;

			// TODO: Handle input layout. For now we expect the user to use one of the globally defined formats.
		}

		void initialize(const char* file, DXRootSignature* root_signature)
		{
			this->pipeline_type = PIPELINE_TYPE_COMPUTE;
			this->compute_desc = {};
			this->compute_file = file;
			this->root_signature = root_signature;
		}
	};

	struct ReloadableRootSignature
	{
		const char* file;
		DXRootSignature root_signature;
	};

	static std::unordered_map<std::string, ShaderFile> shader_blobs;
	static std::deque<ReloadablePipelineState> pipelines;
	static std::deque<ReloadableRootSignature> root_signatures_from_files;
	static std::deque<DXRootSignature> user_root_signatures;

	static std::vector<ReloadablePipelineState*> dirty_pipelines;
	static std::vector<ReloadableRootSignature*> dirty_root_signatures;
	static std::mutex mutex;

	static ReloadableRootSignature* push_blob(const char* filename, ReloadablePipelineState* pipeline_index, bool is_root_signature = false)
	{
		ReloadableRootSignature* result = 0;

		if (filename)
		{
			auto it = shader_blobs.find(filename);
			if (it == shader_blobs.end())
			{
				// New file
				std::wstring filepath = SHADER_BIN_DIR + string_to_wstring(filename) + L".cso";

				DXBlob blob;
				checkResult(D3DReadFileToBlob(filepath.c_str(), &blob));

				if (is_root_signature)
				{
					root_signatures_from_files.push_back({ filename, nullptr });
					result = &root_signatures_from_files.back();
				}

				mutex.lock();
				shader_blobs[filename] = { blob, { pipeline_index }, result };
				mutex.unlock();
			}
			else
			{
				// Already used
				mutex.lock();
				it->second.used_by_pipelines.insert(pipeline_index);
				mutex.unlock();

				if (is_root_signature)
				{
					if (!it->second.root_signature)
					{
						root_signatures_from_files.push_back({ filename, nullptr });
						it->second.root_signature = &root_signatures_from_files.back();
					}

					result = it->second.root_signature;
				}
			}
		}

		return result;
	}

	DXPipeline create_reloadable_pipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const GraphicsPipelineFiles& files,
		DXRootSignature user_root_signature, bool load_immediately)
	{
		auto& state = pipelines.emplace_back();

		push_blob(files.vs, &state);
		push_blob(files.ps, &state);
		push_blob(files.gs, &state);
		push_blob(files.ds, &state);
		push_blob(files.hs, &state);

		ASSERT(!files.ms);
		ASSERT(!files.as);

		user_root_signatures.push_back(user_root_signature);
		DXRootSignature* root_signature = &user_root_signatures.back();
		user_root_signatures.back() = user_root_signature;

		state.initialize(desc, files, root_signature);

		DXPipeline result = { &state.pipeline, root_signature };

		if (load_immediately)
		{
			load_pipeline(state);
		}

		return result;
	}

	DXPipeline create_reloadable_pipeline(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const GraphicsPipelineFiles& files, RSFile root_signature_file, bool load_immediately)
	{
		auto& state = pipelines.emplace_back();

		ReloadableRootSignature* reloadable_rs = push_blob(files.shaders[root_signature_file], &state, true);
		push_blob(files.vs, &state);
		push_blob(files.ps, &state);
		push_blob(files.gs, &state);
		push_blob(files.ds, &state);
		push_blob(files.hs, &state);

		ASSERT(!files.ms);
		ASSERT(!files.as);

		DXRootSignature* root_signature = &reloadable_rs->root_signature;

		state.initialize(desc, files, root_signature);

		DXPipeline result = { &state.pipeline, root_signature };

		if (load_immediately)
		{
			load_root_signature(*reloadable_rs);
			load_pipeline(state);
		}

		return result;
	}

	DXPipeline create_reloadable_pipeline(const char* cs_file, DXRootSignature user_root_signature, bool load_immediately)
	{
		auto& state = pipelines.emplace_back();

		push_blob(cs_file, &state);

		user_root_signatures.push_back(user_root_signature);
		DXRootSignature* root_signature = &user_root_signatures.back();
		user_root_signatures.back() = user_root_signature;

		state.initialize(cs_file, root_signature);

		DXPipeline result = { &state.pipeline, root_signature };

		if (load_immediately)
		{
			load_pipeline(state);
		}

		return result;
	}

	DXPipeline create_reloadable_pipeline(const char* cs_file, bool load_immediately)
	{
		auto& state = pipelines.emplace_back();

		ReloadableRootSignature* reloadable_rs = push_blob(cs_file, &state, true);

		DXRootSignature* root_signature = &reloadable_rs->root_signature;

		state.initialize(cs_file, root_signature);

		DXPipeline result = { &state.pipeline, root_signature };

		if (load_immediately)
		{
			load_root_signature(*reloadable_rs);
			load_pipeline(state);
		}

		return result;
	}

	DXPipeline create_reloadable_pipeline(const D3D12_PIPELINE_STATE_STREAM_DESC& desc, DXPipelineStreamBase* stream, const GraphicsPipelineFiles& files,
		DXRootSignature user_root_signature, bool load_immediately)
	{
		auto& state = pipelines.emplace_back();

		push_blob(files.vs, &state);
		push_blob(files.ps, &state);
		push_blob(files.gs, &state);
		push_blob(files.ds, &state);
		push_blob(files.hs, &state);
		push_blob(files.ms, &state);
		push_blob(files.as, &state);

		user_root_signatures.push_back(user_root_signature);
		DXRootSignature* root_signature = &user_root_signatures.back();
		user_root_signatures.back() = user_root_signature;

		state.initialize(desc, stream, files, root_signature);

		DXPipeline result = { &state.pipeline, root_signature };

		if (load_immediately)
		{
			load_pipeline(state);
		}

		return result;
	}

	DXPipeline create_reloadable_pipeline(const D3D12_PIPELINE_STATE_STREAM_DESC& desc, DXPipelineStreamBase* stream, const GraphicsPipelineFiles& files,
		RSFile root_signature_file, bool load_immediately)
	{
		pipelines.emplace_back();
		auto& state = pipelines.back();

		ReloadableRootSignature* reloadable_rs = push_blob(files.shaders[root_signature_file], &state, true);
		push_blob(files.vs, &state);
		push_blob(files.ps, &state);
		push_blob(files.gs, &state);
		push_blob(files.ds, &state);
		push_blob(files.hs, &state);
		push_blob(files.ms, &state);
		push_blob(files.as, &state);

		DXRootSignature* root_signature = &reloadable_rs->root_signature;

		state.initialize(desc, stream, files, root_signature);

		DXPipeline result = { &state.pipeline, root_signature };

		if (load_immediately)
		{
			load_root_signature(*reloadable_rs);
			load_pipeline(state);
		}

		return result;
	}

	static void load_root_signature(ReloadableRootSignature& r)
	{
		DXBlob rs = shader_blobs[r.file].blob;

		dx_context.retire(r.root_signature.root_signature);
		free_root_signature(r.root_signature);
		r.root_signature = create_root_signature(rs);
	}

	static void load_pipeline(ReloadablePipelineState& p)
	{
		dx_context.retire(p.pipeline);

		if (p.pipeline_type == PIPELINE_TYPE_GRAPHICS)
		{
			if (p.description_type == DESC_TYPE_STRUCT)
			{
				if (p.graphics_files.vs) { DXBlob shader = shader_blobs[p.graphics_files.vs].blob; p.graphics_desc.VS = CD3DX12_SHADER_BYTECODE(shader.Get()); }
				if (p.graphics_files.ps) { DXBlob shader = shader_blobs[p.graphics_files.ps].blob; p.graphics_desc.PS = CD3DX12_SHADER_BYTECODE(shader.Get()); }
				if (p.graphics_files.gs) { DXBlob shader = shader_blobs[p.graphics_files.gs].blob; p.graphics_desc.GS = CD3DX12_SHADER_BYTECODE(shader.Get()); }
				if (p.graphics_files.ds) { DXBlob shader = shader_blobs[p.graphics_files.ds].blob; p.graphics_desc.DS = CD3DX12_SHADER_BYTECODE(shader.Get()); }
				if (p.graphics_files.hs) { DXBlob shader = shader_blobs[p.graphics_files.hs].blob; p.graphics_desc.HS = CD3DX12_SHADER_BYTECODE(shader.Get()); }

				p.graphics_desc.pRootSignature = p.root_signature->root_signature.Get();
				checkResult(dx_context.device->CreateGraphicsPipelineState(&p.graphics_desc, IID_PPV_ARGS(&p.pipeline)));
			}
			else
			{
				if (p.graphics_files.vs) { DXBlob shader = shader_blobs[p.graphics_files.vs].blob; p.stream->set_vertex_shader(shader); }
				if (p.graphics_files.ps) { DXBlob shader = shader_blobs[p.graphics_files.ps].blob; p.stream->set_pixel_shader(shader); }
				if (p.graphics_files.gs) { DXBlob shader = shader_blobs[p.graphics_files.gs].blob; p.stream->set_geometry_shader(shader); }
				if (p.graphics_files.ds) { DXBlob shader = shader_blobs[p.graphics_files.ds].blob; p.stream->set_domain_shader(shader); }
				if (p.graphics_files.hs) { DXBlob shader = shader_blobs[p.graphics_files.hs].blob; p.stream->set_hull_shader(shader); }
				if (p.graphics_files.ms) { DXBlob shader = shader_blobs[p.graphics_files.ms].blob; p.stream->set_mesh_shader(shader); }
				if (p.graphics_files.as) { DXBlob shader = shader_blobs[p.graphics_files.as].blob; p.stream->set_amplification_shader(shader); }

				p.stream->set_root_signature(*p.root_signature);
				checkResult(dx_context.device->CreatePipelineState(&p.stream_desc, IID_PPV_ARGS(&p.pipeline)));
			}
		}
		else
		{
			DXBlob shader = shader_blobs[p.compute_file].blob; p.compute_desc.CS = CD3DX12_SHADER_BYTECODE(shader.Get());

			p.compute_desc.pRootSignature = p.root_signature->root_signature.Get();
			checkResult(dx_context.device->CreateComputePipelineState(&p.compute_desc, IID_PPV_ARGS(&p.pipeline)));
		}
	}

	void create_all_pending_reloadable_pipelines()
	{
		for (uint32 i = 0; i < (uint32)root_signatures_from_files.size(); ++i)
		{
			if (!root_signatures_from_files[i].root_signature.root_signature)
			{
				load_root_signature(root_signatures_from_files[i]);
			}
		}

		for (uint32 i = 0; i < (uint32)pipelines.size(); ++i)
		{
			if (!pipelines[i].pipeline)
			{
				load_pipeline(pipelines[i]);
			}
		}

		//static HANDLE thread = CreateThread(0, 0, checkForFileChanges, 0, 0, 0); // Static, so that we only do this once.
		static bool observing = observe_directory(SHADER_BIN_DIR, handle_pipeline_changes); // Static, so that we only do this once.
	}

	static bool file_is_locked(const wchar* filename)
	{
		HANDLE file_handle = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file_handle == INVALID_HANDLE_VALUE)
			return true;

		CloseHandle(file_handle);
		return false;
	}

	static void handle_pipeline_changes(const FileSystemEvent& e)
	{
		if (e.change == FileSystemChange::MODIFY)
		{
			bool is_file = !fs::is_directory(e.path);

			if (is_file)
			{
				// Lock section
				mutex.lock();
				auto it = shader_blobs.find(e.path.stem().string());
				if (it != shader_blobs.end())
				{
					mutex.unlock();
					auto w_path = e.path.wstring();
					while (file_is_locked(w_path.c_str()))
					{
						// Wait
					}

					uint32 num_dirty_pipelines = (uint32)it->second.used_by_pipelines.size();
					LOG_MESSAGE("Reloading shader blob '%ws'. Updating %u pipeline%s.", e.path.stem().c_str(), num_dirty_pipelines, (num_dirty_pipelines > 1 ? "s" : ""));

					DXBlob blob;
					checkResult(D3DReadFileToBlob(e.path.c_str(), &blob));

					mutex.lock();
					it->second.blob = blob;
					dirty_pipelines.insert(dirty_pipelines.end(), it->second.used_by_pipelines.begin(), it->second.used_by_pipelines.end());
					if (it->second.root_signature)
					{
						dirty_root_signatures.push_back(it->second.root_signature);
					}
					mutex.unlock();
				}
				else
				{
					mutex.unlock();
				}
			}
		}
	}

	void check_for_changed_pipelines()
	{
		Lock lock{ mutex };

		for (uint32 i = 0; i < (uint32)dirty_root_signatures.size(); ++i)
		{
			load_root_signature(*dirty_root_signatures[i]);
		}

		for (uint32 i = 0; i < (uint32)dirty_pipelines.size(); ++i)
		{
			load_pipeline(*dirty_pipelines[i]);
		}

		dirty_root_signatures.clear();
		dirty_pipelines.clear();
	}

	static void copy_root_signature_desc(const D3D12_ROOT_SIGNATURE_DESC* desc, DXRootSignature& result)
	{
		result.total_num_parameters = desc->NumParameters;

		uint32 num_descriptor_tables = 0;
		for (uint32 i = 0; i < desc->NumParameters; ++i)
		{
			if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				++num_descriptor_tables;
				setBit(result.table_root_parameter_mask, i);
			}
		}

		result.descriptor_table_sizes = new uint32[num_descriptor_tables];
		result.num_descriptor_tables = num_descriptor_tables;

		uint32 index = 0;
		for (uint32 i = 0; i < desc->NumParameters; ++i)
		{
			if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				uint32 num_ranges = desc->pParameters[i].DescriptorTable.NumDescriptorRanges;
				result.descriptor_table_sizes[index] = 0;
				for (uint32 r = 0; r < num_ranges; ++r)
				{
					result.descriptor_table_sizes[index] += desc->pParameters[i].DescriptorTable.pDescriptorRanges[r].NumDescriptors;
				}
				++index;
			}
		}
	}

	static void copy_root_signature_desc(const D3D12_ROOT_SIGNATURE_DESC1* desc, DXRootSignature& result)
	{
		result.total_num_parameters = desc->NumParameters;

		uint32 num_descriptor_tables = 0;
		for (uint32 i = 0; i < desc->NumParameters; ++i)
		{
			if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				++num_descriptor_tables;
				setBit(result.table_root_parameter_mask, i);
			}
		}

		result.descriptor_table_sizes = new uint32[num_descriptor_tables];
		result.num_descriptor_tables = num_descriptor_tables;

		uint32 index = 0;
		for (uint32 i = 0; i < desc->NumParameters; ++i)
		{
			if (desc->pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				uint32 num_ranges = desc->pParameters[i].DescriptorTable.NumDescriptorRanges;
				result.descriptor_table_sizes[index] = 0;
				for (uint32 r = 0; r < num_ranges; ++r)
				{
					result.descriptor_table_sizes[index] += desc->pParameters[i].DescriptorTable.pDescriptorRanges[r].NumDescriptors;
				}
				++index;
			}
		}
	}

	NODISCARD DXRootSignature create_root_signature(DXBlob root_signature_blob)
	{
		DXRootSignature result = {};

		checkResult(dx_context.device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
			root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&result.root_signature)));

		com<ID3D12RootSignatureDeserializer> deserializer;
		checkResult(D3D12CreateRootSignatureDeserializer(root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&deserializer)));
		D3D12_ROOT_SIGNATURE_DESC* desc = (D3D12_ROOT_SIGNATURE_DESC*)deserializer->GetRootSignatureDesc();

		copy_root_signature_desc(desc, result);

		return result;
	}

	NODISCARD DXRootSignature create_root_signature(const wchar* path)
	{
		DXBlob root_signature_blob;
		checkResult(D3DReadFileToBlob(path, &root_signature_blob));
		return create_root_signature(root_signature_blob);
	}

	NODISCARD DXRootSignature create_root_signature(const D3D12_ROOT_SIGNATURE_DESC1& desc)
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(dx_context.device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
		{
			feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_description;
		root_signature_description.Init_1_1(desc.NumParameters, desc.pParameters, desc.NumStaticSamplers, desc.pStaticSamplers, desc.Flags);

		DXBlob root_signature_blob;
		DXBlob error_blob;
		checkResult(D3DX12SerializeVersionedRootSignature(&root_signature_description, feature_data.HighestVersion, &root_signature_blob, &error_blob));

		DXRootSignature root_signature = {};

		checkResult(dx_context.device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
			root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature.root_signature)));

		copy_root_signature_desc(&desc, root_signature);

		return root_signature;
	}

	NODISCARD DXRootSignature create_root_signature(CD3DX12_ROOT_PARAMETER1* root_parameters, uint32 num_root_parameters, CD3DX12_STATIC_SAMPLER_DESC* samplers, uint32 num_samplers,
		D3D12_ROOT_SIGNATURE_FLAGS flags)
	{
		D3D12_ROOT_SIGNATURE_DESC1 root_signature_desc = {};
		root_signature_desc.Flags = flags;
		root_signature_desc.pParameters = root_parameters;
		root_signature_desc.NumParameters = num_root_parameters;
		root_signature_desc.pStaticSamplers = samplers;
		root_signature_desc.NumStaticSamplers = num_samplers;
		return create_root_signature(root_signature_desc);
	}

	NODISCARD DXRootSignature create_root_signature(const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		DXBlob root_signature_blob;
		DXBlob error_blob;
		checkResult(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &root_signature_blob, &error_blob));

		DXRootSignature root_signature = {};

		checkResult(dx_context.device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
			root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature.root_signature)));

		copy_root_signature_desc(&desc, root_signature);

		return root_signature;
	}

	NODISCARD DXRootSignature create_root_signature(CD3DX12_ROOT_PARAMETER* root_parameters, uint32 num_root_parameters, CD3DX12_STATIC_SAMPLER_DESC* samplers, uint32 num_samplers,
		D3D12_ROOT_SIGNATURE_FLAGS flags)
	{
		D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
		root_signature_desc.Flags = flags;
		root_signature_desc.pParameters = root_parameters;
		root_signature_desc.NumParameters = num_root_parameters;
		root_signature_desc.pStaticSamplers = samplers;
		root_signature_desc.NumStaticSamplers = num_samplers;
		return create_root_signature(root_signature_desc);
	}

	NODISCARD DXRootSignature create_root_signature(D3D12_ROOT_SIGNATURE_FLAGS flags)
	{
		D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
		root_signature_desc.Flags = flags;
		return create_root_signature(root_signature_desc);
	}

	void free_root_signature(DXRootSignature& rs)
	{
		if (rs.descriptor_table_sizes)
		{
			delete[] rs.descriptor_table_sizes;
		}
	}

	DXCommandSignature create_command_signature(DXRootSignature root_signature, const D3D12_COMMAND_SIGNATURE_DESC& command_signature_desc)
	{
		DXCommandSignature command_signature;
		checkResult(dx_context.device->CreateCommandSignature(&command_signature_desc,
			command_signature_desc.NumArgumentDescs == 1 ? 0 : root_signature.root_signature.Get(),
			IID_PPV_ARGS(&command_signature)));
		return command_signature;
	}

	DXCommandSignature create_command_signature(DXRootSignature root_signature, D3D12_INDIRECT_ARGUMENT_DESC* argument_descs, uint32 num_argument_descs, uint32 command_structure_size)
	{
		D3D12_COMMAND_SIGNATURE_DESC command_signature_desc;
		command_signature_desc.pArgumentDescs = argument_descs;
		command_signature_desc.NumArgumentDescs = num_argument_descs;
		command_signature_desc.ByteStride = command_structure_size;
		command_signature_desc.NodeMask = 0;

		return create_command_signature(root_signature, command_signature_desc);
	}
}