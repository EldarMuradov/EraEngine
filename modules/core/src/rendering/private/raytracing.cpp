// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "rendering/raytracing.h"

#include "core/string.h"

#include "dx/dx_context.h"
#include "dx/dx_command_list.h"

#include <rttr/registration>

#include <dxcapi.h>

#include <fstream>
#include <sstream>

namespace era_engine
{
	RaytracingBLASBuilder& RaytracingBLASBuilder::push(VertexBufferGroup vertex_buffer, ref<DXIndexBuffer> index_buffer, SubmeshInfo submesh, bool opaque, const trs& local_transform)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC geom_desc;

		geom_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geom_desc.Flags = opaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

		if (&local_transform == &trs::identity)
		{
			geom_desc.Triangles.Transform3x4 = UINT64_MAX;
		}
		else
		{
			geom_desc.Triangles.Transform3x4 = local_transforms.size();
			local_transforms.push_back(transpose(trs_to_mat4(local_transform)));
		}

		geom_desc.Triangles.VertexBuffer.StartAddress = vertex_buffer.positions->gpu_virtual_address + (vertex_buffer.positions->element_size * submesh.base_vertex);
		geom_desc.Triangles.VertexBuffer.StrideInBytes = vertex_buffer.positions->element_size;
		geom_desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geom_desc.Triangles.VertexCount = submesh.num_vertices;

		geom_desc.Triangles.IndexBuffer = index_buffer->gpu_virtual_address + (index_buffer->element_size * submesh.first_index);
		geom_desc.Triangles.IndexFormat = get_index_buffer_format(index_buffer->element_size);
		geom_desc.Triangles.IndexCount = submesh.num_indices;

		geometry_descs.push_back(geom_desc);
		geometries.push_back({ RAYTRACING_MESH_GEOMETRY, vertex_buffer, index_buffer, submesh });

		return *this;
	}

	RaytracingBLASBuilder& RaytracingBLASBuilder::push(const std::vector<BoundingBox>& bounding_boxes, bool opaque)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC geom_desc;

		geom_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
		geom_desc.Flags = opaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

		for (uint32 i = 0; i < (uint32)bounding_boxes.size(); ++i)
		{
			D3D12_RAYTRACING_AABB& w = aabb_descs.emplace_back();
			const BoundingBox& r = bounding_boxes[i];

			w.MinX = r.min_corner.x;
			w.MinY = r.min_corner.y;
			w.MinZ = r.min_corner.z;

			w.MaxX = r.max_corner.x;
			w.MaxY = r.max_corner.y;
			w.MaxZ = r.max_corner.z;
		}

		geom_desc.AABBs.AABBCount = bounding_boxes.size();

		geometry_descs.push_back(geom_desc);
		geometries.push_back({ RAYTRACING_PROCEDURAL_GEOMETRY });

		return *this;
	}

	NODISCARD ref<RaytracingBLAS> RaytracingBLASBuilder::finish(bool keep_scratch)
	{
		DXDynamicConstantBuffer local_transforms_buffer = {};
		if (local_transforms.size() > 0)
		{
			local_transforms_buffer = dx_context.upload_dynamic_constant_buffer(sizeof(mat4) * (uint32)local_transforms.size(), local_transforms.data()).first;
		}

		DXDynamicConstantBuffer aabb_buffer = {};
		if (aabb_descs.size())
		{
			aabb_buffer = dx_context.upload_dynamic_constant_buffer(sizeof(D3D12_RAYTRACING_AABB) * (uint32)aabb_descs.size(), aabb_descs.data()).first;
		}

		uint64 aabb_offset = 0;

		for (auto& desc : geometry_descs)
		{
			if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES)
			{
				if (desc.Triangles.Transform3x4 == UINT64_MAX)
				{
					desc.Triangles.Transform3x4 = 0;
				}
				else
				{
					desc.Triangles.Transform3x4 = local_transforms_buffer.gpu_ptr + sizeof(mat4) * desc.Triangles.Transform3x4;
				}
			}
			else if (desc.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS)
			{
				desc.AABBs.AABBs.StartAddress = aabb_buffer.gpu_ptr + sizeof(D3D12_RAYTRACING_AABB) * aabb_offset;
				desc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
				aabb_offset += desc.AABBs.AABBCount;
			}
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

		inputs.NumDescs = (uint32)geometry_descs.size();
		inputs.pGeometryDescs = geometry_descs.data();

		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		// Allocate
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		dx_context.device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		info.ScratchDataSizeInBytes = align_to(info.ScratchDataSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		info.ResultDataMaxSizeInBytes = align_to(info.ResultDataMaxSizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		ref<RaytracingBLAS> blas = make_ref<RaytracingBLAS>();
		blas->scratch = create_buffer((uint32)info.ScratchDataSizeInBytes, 1, 0, true, false);
		blas->blas = create_buffer((uint32)info.ResultDataMaxSizeInBytes, 1, 0, true, false, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

		SET_NAME(blas->scratch->resource, "BLAS Scratch");
		SET_NAME(blas->blas->resource, "BLAS Result");

		blas->geometries = std::move(geometries);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC as_desc = {};
		as_desc.Inputs = inputs;
		as_desc.DestAccelerationStructureData = blas->blas->gpu_virtual_address;
		as_desc.ScratchAccelerationStructureData = blas->scratch->gpu_virtual_address;

		DXCommandList* cl = dx_context.get_free_compute_command_list(true);
		cl->command_list->BuildRaytracingAccelerationStructure(&as_desc, 0, 0);
		cl->uav_barrier(blas->blas);
		dx_context.execute_command_list(cl);

		if (!keep_scratch)
		{
			blas->scratch = 0;
		}

		return blas;
	}

	static void report_shader_compile_error(com<IDxcBlobEncoding> blob)
	{
		char info_log[2048];
		memcpy(info_log, blob->GetBufferPointer(), sizeof(info_log) - 1);
		info_log[sizeof(info_log) - 1] = 0;
		std::cerr << "Error:\n" << info_log << '\n';
	}

	static com<IDxcBlob> compile_library(const std::wstring& filename, const std::vector<const wchar*>& shader_name_defines)
	{
		com<IDxcCompiler> compiler;
		com<IDxcLibrary> library;
		com<IDxcIncludeHandler> include_handler;
		checkResult(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
		checkResult(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
		checkResult(library->CreateIncludeHandler(&include_handler));

		std::string source = "#include \"" + wstring_to_string(filename) + "\"\n";

		// Create blob from the string.
		com<IDxcBlobEncoding> text_blob;
		checkResult(library->CreateBlobWithEncodingFromPinned((LPBYTE)source.c_str(), (uint32)source.length(), 0, &text_blob));

		std::vector<DxcDefine> defines;

		std::vector<std::wstring> value_strings;
		value_strings.reserve(1 + shader_name_defines.size()); // Important. We pull out raw char pointers from these, so the vector should not reallocate.

		value_strings.push_back(std::to_wstring(shader_name_defines.size()));
		defines.push_back({ L"NUM_RAY_TYPES", value_strings.back().c_str() });

		for (uint32 i = 0; i < shader_name_defines.size(); ++i)
		{
			value_strings.push_back(std::to_wstring(i));
			defines.push_back({ shader_name_defines[i], value_strings.back().c_str() });
		}

		defines.push_back({ L"HLSL" });
		defines.push_back({ L"mat4",	L"float4x4" });
		defines.push_back({ L"vec2",	L"float2" });
		defines.push_back({ L"vec3",	L"float3" });
		defines.push_back({ L"vec4",	L"float4" });
		defines.push_back({ L"uint32",	L"uint" });

		// Compile
		com<IDxcOperationResult> operation_result;
		checkResult(compiler->Compile(text_blob.Get(), 0, L"", L"lib_6_3", 0, 0, defines.data(), (uint32)defines.size(), include_handler.Get(), &operation_result));

		// Verify the result
		HRESULT result_code;
		checkResult(operation_result->GetStatus(&result_code));
		if (FAILED(result_code))
		{
			com<IDxcBlobEncoding> error;
			checkResult(operation_result->GetErrorBuffer(&error));
			report_shader_compile_error(error);
			ASSERT(false);
			return 0;
		}

		com<IDxcBlob> blob;
		checkResult(operation_result->GetResult(&blob));

		return blob;
	}

	RaytracingPipelineBuilder::RaytracingPipelineBuilder(const wchar* shader_filename, uint32 payload_size, uint32 max_recursion_depth, bool has_mesh_geometry, bool has_procedural_geometry)
	{
		this->shader_filename = shader_filename;
		this->payload_size = payload_size;
		this->max_recursion_depth = max_recursion_depth;
		this->has_mesh_geometry = has_mesh_geometry;
		this->has_procedural_geometry = has_procedural_geometry;

		ASSERT(has_mesh_geometry || has_procedural_geometry);
	}

	RaytracingPipelineBuilder::RaytracingRootSignature RaytracingPipelineBuilder::create_raytracing_root_signature(const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		RaytracingRootSignature result;
		result.root_signature = create_root_signature(desc);
		result.root_signature_ptr = result.root_signature.root_signature.Get();
		return result;
	}

	RaytracingPipelineBuilder& RaytracingPipelineBuilder::global_root_signature(D3D12_ROOT_SIGNATURE_DESC root_signature_desc)
	{
		ASSERT(!global_rs.root_signature.root_signature);

		global_rs = create_raytracing_root_signature(root_signature_desc);
		SET_NAME(global_rs.root_signature.root_signature, "Global raytracing root signature");

		auto& so = subobjects[num_subobjects++];
		so.pDesc = &global_rs.root_signature_ptr;
		so.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;

		return *this;
	}

	static uint32 get_shader_binding_table_size(const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc)
	{
		uint32 size = 0;
		for (uint32 i = 0; i < root_signature_desc.NumParameters; ++i)
		{
			if (root_signature_desc.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
			{
				size += align_to(root_signature_desc.pParameters[i].Constants.Num32BitValues * 4, 8);
			}
			else
			{
				size += 8;
			}
		}
		return size;
	}

	RaytracingPipelineBuilder& RaytracingPipelineBuilder::raygen(const wchar* entry_point, D3D12_ROOT_SIGNATURE_DESC root_signature_desc)
	{
		ASSERT(!raygen_rs.root_signature.root_signature);

		D3D12_EXPORT_DESC& exp = exports[num_exports++];
		exp.Name = entry_point;
		exp.Flags = D3D12_EXPORT_FLAG_NONE;
		exp.ExportToRename = 0;

		raygen_entry_point = entry_point;

		root_signature_desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		raygen_rs = create_raytracing_root_signature(root_signature_desc);
		SET_NAME(raygen_rs.root_signature.root_signature, "Local raytracing root signature");

		{
			auto& so = subobjects[num_subobjects++];
			so.pDesc = &raygen_rs.root_signature_ptr;
			so.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		}

		{
			auto& so = subobjects[num_subobjects++];
			auto& as = associations[num_associations++];

			string_buffer[num_strings++] = entry_point;

			as.NumExports = 1;
			as.pExports = &string_buffer[num_strings - 1];
			as.pSubobjectToAssociate = &subobjects[num_subobjects - 2];

			so.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			so.pDesc = &as;
		}

		all_exports.push_back(entry_point);

		uint32 size = get_shader_binding_table_size(root_signature_desc);
		table_entry_size = max(size, table_entry_size);

		return *this;
	}

	RaytracingPipelineBuilder& RaytracingPipelineBuilder::hitgroup(const wchar* group_name, const wchar* miss,
		RaytracingMeshHitgroup mesh, D3D12_ROOT_SIGNATURE_DESC mesh_root_signature_desc,
		RaytracingProceduralHitgroup procedural, D3D12_ROOT_SIGNATURE_DESC procedural_root_signature_desc)
	{
		auto export_entry_point = [this](const wchar* entry_point)
			{
				D3D12_EXPORT_DESC& exp = exports[num_exports++];
				exp.Name = entry_point;
				exp.Flags = D3D12_EXPORT_FLAG_NONE;
				exp.ExportToRename = 0;

				all_exports.push_back(entry_point);
			};

		auto create_local_root_signature = [this](D3D12_ROOT_SIGNATURE_DESC root_signature_desc, const wchar* closest_hit, const wchar* any_hit, const wchar* intersection)
			{
				if (root_signature_desc.NumParameters > 0)
				{
					root_signature_desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
					root_signatures[num_root_signatures++] = create_raytracing_root_signature(root_signature_desc);

					{
						auto& so = subobjects[num_subobjects++];
						so.pDesc = &root_signatures[num_root_signatures - 1].root_signature_ptr;
						so.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
					}

					{
						auto& so = subobjects[num_subobjects++];
						auto& as = associations[num_associations++];

						const wchar** entry_points = &string_buffer[num_strings];
						uint32 num_entry_points = 0;

						if (closest_hit) { entry_points[num_entry_points++] = closest_hit; }
						if (any_hit) { entry_points[num_entry_points++] = any_hit; }
						if (intersection) { entry_points[num_entry_points++] = intersection; }

						num_strings += num_entry_points;

						as.NumExports = num_entry_points;
						as.pExports = entry_points;
						as.pSubobjectToAssociate = &subobjects[num_subobjects - 2];

						so.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
						so.pDesc = &as;
					}

					uint32 size = get_shader_binding_table_size(root_signature_desc);
					table_entry_size = max(size, table_entry_size);
				}
			};

		if (has_mesh_geometry)
		{
			group_name_storage[group_name_storage_ptr++] = std::wstring(group_name) + L"_MESH";
			const wchar* name = group_name_storage[group_name_storage_ptr - 1].c_str();

			D3D12_HIT_GROUP_DESC& hit_group = hit_groups[num_hit_groups++];

			hit_group.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
			hit_group.AnyHitShaderImport = mesh.any_hit;
			hit_group.ClosestHitShaderImport = mesh.closest_hit;
			hit_group.IntersectionShaderImport = 0;
			hit_group.HitGroupExport = name;

			auto& so = subobjects[num_subobjects++];
			so.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			so.pDesc = &hit_group;

			if (mesh.closest_hit) { export_entry_point(mesh.closest_hit); }
			if (mesh.any_hit) { export_entry_point(mesh.any_hit); }

			create_local_root_signature(mesh_root_signature_desc, mesh.closest_hit, mesh.any_hit, 0);
		}

		if (has_procedural_geometry)
		{
			group_name_storage[group_name_storage_ptr++] = std::wstring(group_name) + L"_PROC";
			const wchar* name = group_name_storage[group_name_storage_ptr - 1].c_str();

			D3D12_HIT_GROUP_DESC& hit_group = hit_groups[num_hit_groups++];

			hit_group.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
			hit_group.AnyHitShaderImport = procedural.any_hit;
			hit_group.ClosestHitShaderImport = procedural.closest_hit;
			hit_group.IntersectionShaderImport = procedural.intersection;
			hit_group.HitGroupExport = name;

			auto& so = subobjects[num_subobjects++];
			so.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			so.pDesc = &hit_group;

			ASSERT(procedural.intersection);

			if (procedural.closest_hit) { export_entry_point(procedural.closest_hit); }
			if (procedural.intersection) { export_entry_point(procedural.intersection); }
			if (procedural.any_hit) { export_entry_point(procedural.any_hit); }

			create_local_root_signature(mesh_root_signature_desc, procedural.closest_hit, procedural.any_hit, procedural.intersection);
		}

		export_entry_point(miss);
		empty_associations.push_back(miss);
		miss_entry_points.push_back(miss);

		shader_name_defines.push_back(group_name);

		return *this;
	}

	DXRaytracingPipeline RaytracingPipelineBuilder::finish()
	{
		ASSERT(raygen_rs.root_signature.root_signature);
		ASSERT(global_rs.root_signature.root_signature);

		auto shader_blob = compile_library(shader_filename, shader_name_defines);

		D3D12_DXIL_LIBRARY_DESC dxil_lib_desc;
		dxil_lib_desc.DXILLibrary.pShaderBytecode = shader_blob->GetBufferPointer();
		dxil_lib_desc.DXILLibrary.BytecodeLength = shader_blob->GetBufferSize();
		dxil_lib_desc.NumExports = num_exports;
		dxil_lib_desc.pExports = exports;

		D3D12_ROOT_SIGNATURE_DESC empty_root_signature_desc = {};
		empty_root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		RaytracingRootSignature empty_root_signature = create_raytracing_root_signature(empty_root_signature_desc);

		D3D12_RAYTRACING_SHADER_CONFIG shader_config;
		D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config;

		{
			D3D12_STATE_SUBOBJECT& so = subobjects[num_subobjects++];
			so.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
			so.pDesc = &dxil_lib_desc;
		}

		{
			D3D12_STATE_SUBOBJECT& so = subobjects[num_subobjects++];
			so.pDesc = &empty_root_signature.root_signature_ptr;
			so.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		}

		{
			auto& so = subobjects[num_subobjects++];
			auto& as = associations[num_associations++];

			as.NumExports = (uint32)empty_associations.size();
			as.pExports = empty_associations.data();
			as.pSubobjectToAssociate = &subobjects[num_subobjects - 2];

			so.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			so.pDesc = &as;
		}

		{
			shader_config.MaxAttributeSizeInBytes = sizeof(float) * 2; // 2 floats for the BuiltInTriangleIntersectionAttributes.
			shader_config.MaxPayloadSizeInBytes = payload_size;

			auto& so = subobjects[num_subobjects++];
			so.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
			so.pDesc = &shader_config;
		}

		{
			auto& so = subobjects[num_subobjects++];
			auto& as = associations[num_associations++];

			as.NumExports = (uint32)all_exports.size();
			as.pExports = all_exports.data();
			as.pSubobjectToAssociate = &subobjects[num_subobjects - 2];

			so.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			so.pDesc = &as;
		}

		{
			pipeline_config.MaxTraceRecursionDepth = max_recursion_depth;

			auto& so = subobjects[num_subobjects++];
			so.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
			so.pDesc = &pipeline_config;
		}

		D3D12_STATE_OBJECT_DESC desc;
		desc.NumSubobjects = num_subobjects;
		desc.pSubobjects = subobjects;
		desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

		DXRaytracingPipeline result;
		checkResult(dx_context.device->CreateStateObject(&desc, IID_PPV_ARGS(&result.pipeline)));
		result.root_signature = global_rs.root_signature;

		for (uint32 i = 0; i < num_root_signatures; ++i)
		{
			free_root_signature(root_signatures[i].root_signature);
		}
		free_root_signature(raygen_rs.root_signature);

		com<ID3D12StateObjectProperties> rtso_props;
		result.pipeline->QueryInterface(IID_PPV_ARGS(&rtso_props));

		{
			auto& shader_binding_table_desc = result.shader_binding_table_desc;
			table_entry_size += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			shader_binding_table_desc.entry_size = (uint32)align_to(table_entry_size, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

			uint32 num_geometry_types = has_mesh_geometry + has_procedural_geometry;

			uint32 num_raygen_shader_entries = 1;
			uint32 num_miss_shader_entries = num_hit_groups / num_geometry_types;

			shader_binding_table_desc.raygen = rtso_props->GetShaderIdentifier(raygen_entry_point);

			uint32 num_unique_groups = (uint32)miss_entry_points.size();
			ASSERT(num_unique_groups * num_geometry_types == num_hit_groups);

			for (uint32 i = 0; i < num_unique_groups; ++i)
			{
				shader_binding_table_desc.miss.push_back(rtso_props->GetShaderIdentifier(miss_entry_points[i]));

				RaytracingShader& shader = shader_binding_table_desc.hit_groups.emplace_back();
				shader = { };
				if (has_mesh_geometry)
				{
					shader.mesh = rtso_props->GetShaderIdentifier(hit_groups[num_geometry_types * i].HitGroupExport);
				}
				if (has_procedural_geometry)
				{
					shader.procedural = rtso_props->GetShaderIdentifier(hit_groups[num_geometry_types * i + num_geometry_types - 1].HitGroupExport);
				}
			}

			shader_binding_table_desc.raygen_offset = 0;
			shader_binding_table_desc.miss_offset = shader_binding_table_desc.raygen_offset + (uint32)align_to(num_raygen_shader_entries * table_entry_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
			shader_binding_table_desc.hit_offset = shader_binding_table_desc.miss_offset + (uint32)align_to(num_miss_shader_entries * table_entry_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
		}

		return result;
	}

	RTTR_REGISTRATION
	{
		using namespace rttr;
		rttr::registration::class_<RaytraceComponent>("RaytraceComponent")
			.constructor<>()
			.constructor<ref<Entity::EcsData>, const RaytracingObjectType&>()
			.property("type", &RaytraceComponent::type);
	}

	RaytraceComponent::RaytraceComponent(ref<Entity::EcsData> _data, const RaytracingObjectType& _type)
		:  Component(_data), type(_type)
	{
	}

	RaytraceComponent::~RaytraceComponent()
	{
	}

}