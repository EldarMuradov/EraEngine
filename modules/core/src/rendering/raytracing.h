// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"

#include "geometry/mesh.h"

#include "ecs/component.h"

#include "dx/dx_buffer.h"
#include "dx/dx_pipeline.h"

namespace era_engine
{
	enum RaytracingASRebuildMode
	{
		RAYTRACING_AS_REBUILD,
		RAYTRACING_AS_REFIT,
	};

	enum RaytracingGeometryType
	{
		RAYTRACING_MESH_GEOMETRY,
		RAYTRACING_PROCEDURAL_GEOMETRY,
	};

	struct RaytracingBLASGeometry
	{
		RaytracingGeometryType type;

		// Only valid for mesh geometry
		VertexBufferGroup vertex_buffer;
		ref<DXIndexBuffer> index_buffer;
		SubmeshInfo submesh;
	};

	struct RaytracingBLAS
	{
		ref<DXBuffer> scratch;
		ref<DXBuffer> blas;

		std::vector<RaytracingBLASGeometry> geometries;
	};

	struct RaytracingBLASBuilder
	{
		RaytracingBLASBuilder& push(VertexBufferGroup vertex_buffer, ref<DXIndexBuffer> index_buffer, SubmeshInfo submesh, bool opaque = true, const trs& local_transform = trs::identity);
		RaytracingBLASBuilder& push(const std::vector<BoundingBox>& bounding_boxes, bool opaque);
		NODISCARD ref<RaytracingBLAS> finish(bool keep_scratch = false);

	private:
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometry_descs;

		std::vector<RaytracingBLASGeometry> geometries;

		std::vector<mat4> local_transforms;				// For meshes
		std::vector<D3D12_RAYTRACING_AABB> aabb_descs;	// For procedurals
	};

	struct RaytracingObjectType
	{
		ref<RaytracingBLAS> blas;
		uint32 instance_contribution_to_hit_group_index;
	};

	class RaytraceComponent final : public Component
	{
	public:
		RaytraceComponent() = default;
		RaytraceComponent(ref<Entity::EcsData> _data, const RaytracingObjectType& _type);
		virtual ~RaytraceComponent();

		ERA_VIRTUAL_REFLECT(Component)
	public:
		RaytracingObjectType type;
	};

	struct RaytracingShader
	{
		void* mesh;
		void* procedural;
	};

	struct RaytracingShaderBindingTableDesc
	{
		uint32 entry_size;

		void* raygen;
		std::vector<void*> miss;
		std::vector<RaytracingShader> hit_groups;

		uint32 raygen_offset;
		uint32 miss_offset;
		uint32 hit_offset;
	};

	struct DXRaytracingPipeline
	{
		DXRaytracingPipelineState pipeline;
		DXRootSignature root_signature;

		RaytracingShaderBindingTableDesc shader_binding_table_desc;
	};

	struct RaytracingMeshHitgroup
	{
		const wchar* closest_hit;	// Optional
		const wchar* any_hit;		// Optional
	};

	struct RaytracingProceduralHitgroup
	{
		const wchar* intersection;
		const wchar* closest_hit;	// Optional
		const wchar* any_hit;		// Optional
	};

	struct RaytracingPipelineBuilder
	{
		RaytracingPipelineBuilder(const wchar* shader_filename, uint32 payload_size, uint32 max_recursion_depth, bool has_mesh_geometry, bool has_procedural_geometry);

		RaytracingPipelineBuilder& global_root_signature(D3D12_ROOT_SIGNATURE_DESC root_signature_desc);
		RaytracingPipelineBuilder& raygen(const wchar* entry_point, D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {});

		// The root signature describes parameters for both hit shaders. Miss will not get any arguments for now
		RaytracingPipelineBuilder& hitgroup(const wchar* group_name, const wchar* miss,
			RaytracingMeshHitgroup mesh, D3D12_ROOT_SIGNATURE_DESC mesh_root_signature_desc = {},
			RaytracingProceduralHitgroup procedural = {}, D3D12_ROOT_SIGNATURE_DESC procedural_root_signature_desc = {});

		DXRaytracingPipeline finish();

	private:
		struct RaytracingRootSignature
		{
			DXRootSignature root_signature;
			ID3D12RootSignature* root_signature_ptr;
		};

		RaytracingRootSignature create_raytracing_root_signature(const D3D12_ROOT_SIGNATURE_DESC& desc);

		RaytracingRootSignature global_rs;
		RaytracingRootSignature raygen_rs;

		std::vector<const wchar*> empty_associations;
		std::vector<const wchar*> all_exports;

		const wchar* raygen_entry_point;
		std::vector<const wchar*> miss_entry_points;

		std::vector<const wchar*> shader_name_defines;

		uint32 payload_size;
		uint32 max_recursion_depth;

		bool has_mesh_geometry;
		bool has_procedural_geometry;

		uint32 table_entry_size = 0;

		const wchar* shader_filename;

		// Since these store pointers to each other, they are not resizable arrays
		D3D12_STATE_SUBOBJECT subobjects[512];
		uint32 num_subobjects = 0;

		D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION associations[16];
		uint32 num_associations = 0;

		D3D12_HIT_GROUP_DESC hit_groups[8];
		uint32 num_hit_groups = 0;

		D3D12_EXPORT_DESC exports[24];
		uint32 num_exports = 0;

		const wchar* string_buffer[128];
		uint32 num_strings = 0;

		RaytracingRootSignature root_signatures[8];
		uint32 num_root_signatures = 0;

		std::wstring group_name_storage[8];
		uint32 group_name_storage_ptr = 0;
	};
}