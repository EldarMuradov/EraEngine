#pragma once

#include "core_api.h"

#include "core/memory.h"
#include "core/bounding_volumes.h"

#include "animation/animation_common_data.h"

#include "dx/dx_buffer.h"

namespace era_engine
{
	struct ERA_CORE_API SubmeshInfo
	{
		uint32 num_indices;
		uint32 first_index;
		uint32 base_vertex;
		uint32 num_vertices;
	};

	// Members are always pushed in this order!
	enum MeshCreationFlags
	{
		MESH_CREATION_FLAGS_NONE = 0,
		MESH_CREATION_FLAGS_WITH_POSITIONS = (1 << 0),
		MESH_CREATION_FLAGS_WITH_UVS = (1 << 1),
		MESH_CREATION_FLAGS_WITH_NORMALS = (1 << 2),
		MESH_CREATION_FLAGS_WITH_TANGENTS = (1 << 3),
		MESH_CREATION_FLAGS_WITH_SKIN = (1 << 4),
		MESH_CREATION_FLAGS_WITH_COLORS = (1 << 5),
		MESH_CREATION_FLAGS_SM_TO_M = (1 << 6),
		MESH_CREATION_FLAGS_M_TO_SM = (1 << 7),
		MESH_CREATION_FLAGS_YZX_TO_XYZ = (1 << 8),
		MESH_CREATION_FLAGS_GENERATE_ROOT_MOTION = (1 << 9),
		MESH_CREATION_FLAGS_OVERRIDE_TEXTURES_WITH_LOCAL_FOLDER = (1 << 10),
		MESH_CREATION_FLAGS_FLIP_X = (1 << 11),
		MESH_CREATION_FLAGS_COMPACT = (1 << 12),

		MESH_CREATION_FLAGS_DEFAULT = MESH_CREATION_FLAGS_WITH_POSITIONS | MESH_CREATION_FLAGS_WITH_UVS | MESH_CREATION_FLAGS_WITH_NORMALS | MESH_CREATION_FLAGS_WITH_TANGENTS,
		MESH_CREATION_FLAGS_ANIMATED = MESH_CREATION_FLAGS_DEFAULT | MESH_CREATION_FLAGS_WITH_SKIN,
		MESH_CREATION_FLAGS_UNREAL_ASSET = MESH_CREATION_FLAGS_DEFAULT | MESH_CREATION_FLAGS_SM_TO_M | MESH_CREATION_FLAGS_YZX_TO_XYZ,
		MESH_CREATION_FLAGS_UNREAL_ANIMATED_ASSET = MESH_CREATION_FLAGS_UNREAL_ASSET | MESH_CREATION_FLAGS_WITH_SKIN
	};

	enum MeshIndexType
	{
		MESH_INDEX_UINT16,
		MESH_INDEX_UINT32,
	};

	struct ERA_CORE_API QuadMeshDesc
	{
		vec3 center = 0.f;
		vec2 radius = 1.f;
		quat rotation = quat::identity;
	};

	struct ERA_CORE_API BoxMeshDesc
	{
		vec3 center = 0.f;
		vec3 radius = 1.f;
		quat rotation = quat::identity;
	};

	struct ERA_CORE_API TesselatedBoxMeshDesc
	{
		vec3 center = 0.f;
		vec3 radius = 1.f;
		uint32 num_intervals = 1;
		quat rotation = quat::identity;
	};

	struct ERA_CORE_API SphereMeshDesc
	{
		vec3 center = 0.f;
		float radius = 1.f;
		uint32 slices = 15;
		uint32 rows = 15;
	};

	struct ERA_CORE_API IcosphereMeshDesc
	{
		vec3 center = 0.f;
		float radius = 1.f;
		uint32 refinement = 0;
	};

	// All meshes are standing upright by default.
	struct ERA_CORE_API CapsuleMeshDesc
	{
		CapsuleMeshDesc() {}

		CapsuleMeshDesc(vec3 pos_a, vec3 pos_b, float _radius)
		{
			center = (pos_a + pos_b) * 0.5f;
			height = length(pos_a - pos_b);
			radius = _radius;
			rotation = rotate_from_to(vec3(0.f, 1.f, 0.f), pos_b - pos_a);
		}

		vec3 center = 0.f;
		float height = 1.f;
		float radius = 0.4f;
		quat rotation = quat::identity;
		uint32 slices = 15;
		uint32 rows = 15;
	};

	struct ERA_CORE_API CylinderMeshDesc
	{
		vec3 center = 0.f;
		float height = 1.f;
		float radius = 0.4f;
		quat rotation = quat::identity;
		uint32 slices = 15;
	};

	struct ERA_CORE_API HollowCylinderMeshDesc
	{
		vec3 center = 0.f;
		float height = 1.f;
		float radius = 0.4f;
		float inner_radius = 0.3f;
		quat rotation = quat::identity;
		uint32 slices = 15;
	};

	struct ERA_CORE_API ArrowMeshDesc
	{
		vec3 base = 0.f;
		float shaft_length = 1.f;
		float shaft_radius = 0.1f;
		float head_length = 0.3f;
		float head_radius = 0.2f;
		quat rotation = quat::identity;
		uint32 slices = 15;
	};

	struct ERA_CORE_API TorusMeshDesc
	{
		vec3 center = 0.f;
		float torus_radius = 1.f;
		float tube_radius = 0.1f;
		quat rotation = quat::identity;
		uint32 slices = 15;
		uint32 segments = 15;
	};

	struct ERA_CORE_API MaceMeshDesc
	{
		vec3 base = 0.f;
		float shaft_length = 1.f;
		float shaft_radius = 0.1f;
		float head_length = 0.3f;
		float head_radius = 0.2f;
		quat rotation = quat::identity;
		uint32 slices = 15;
	};

	NODISCARD inline constexpr uint32 get_vertex_others_size(uint32 mesh_flags)
	{
		uint32 size = 0;
		if (mesh_flags & MESH_CREATION_FLAGS_WITH_UVS) { size += sizeof(vec2); }
		if (mesh_flags & MESH_CREATION_FLAGS_WITH_NORMALS) { size += sizeof(vec3); }
		if (mesh_flags & MESH_CREATION_FLAGS_WITH_TANGENTS) { size += sizeof(vec3); }
		if (mesh_flags & MESH_CREATION_FLAGS_WITH_SKIN) { size += sizeof(era_engine::animation::SkinningWeights); }
		if (mesh_flags & MESH_CREATION_FLAGS_WITH_COLORS) { size += sizeof(uint32); }
		return size;
	}

	struct ERA_CORE_API MeshBuilder
	{
		MeshBuilder(uint32 vertex_flags = MESH_CREATION_FLAGS_DEFAULT, MeshIndexType index_type = MESH_INDEX_UINT32);
		MeshBuilder(const MeshBuilder& mesh) = delete;
		MeshBuilder(MeshBuilder&& mesh) = default;
		~MeshBuilder();

		void push_quad(const QuadMeshDesc& desc, bool flip_winding_order = false);
		void push_box(const BoxMeshDesc& desc, bool flip_winding_order = false);
		void push_tesselated_box(const TesselatedBoxMeshDesc& desc, bool flip_winding_order = false);
		void push_sphere(const SphereMeshDesc& desc, bool flip_winding_order = false);
		void push_ico_sphere(const IcosphereMeshDesc& desc, bool flip_winding_order = false);
		void push_capsule(const CapsuleMeshDesc& desc, bool flip_winding_order = false);
		void push_cylinder(const CylinderMeshDesc& desc, bool flip_winding_order = false);
		void push_hollow_cylinder(const HollowCylinderMeshDesc& desc, bool flip_winding_order = false);
		void push_arrow(const ArrowMeshDesc& desc, bool flip_winding_order = false);
		void push_torus(const TorusMeshDesc& desc, bool flip_winding_order = false);
		void push_mace(const MaceMeshDesc& desc, bool flip_winding_order = false);

		void push_mesh(const struct SubmeshAsset& mesh, float scale, BoundingBox* aabb = 0);

		SubmeshInfo end_submesh();

		DXMesh create_dx_mesh();

		NODISCARD vec3* get_positions() { return (vec3*)position_arena.base(); }
		NODISCARD void* get_others() { return others_arena.base(); }
		NODISCARD void* get_triangles() { return index_arena.base(); }

		NODISCARD uint32 get_num_vertices() const { return total_num_vertices; }
		NODISCARD uint32 get_num_triangles() const { return total_num_triangles; }
		NODISCARD uint32 get_skin_offset() const { return skin_offset; }
		NODISCARD uint32 get_others_size() const { return others_size; }

	private:
		std::tuple<vec3*, uint8*, uint8*, uint32> begin_primitive(uint32 num_vertices, uint32 num_triangles);

		Allocator position_arena;
		Allocator others_arena;
		Allocator index_arena;

		uint32 vertex_flags;
		MeshIndexType index_type;
		uint32 others_size;
		uint32 index_size;
		uint32 skin_offset;

		uint32 total_num_vertices = 0;
		uint32 total_num_triangles = 0;

		uint32 num_vertices_in_current_submesh = 0;
		uint32 num_triangles_in_current_submesh = 0;

		uint32 num_submeshes = 0;
	};

	struct VertexUVNormalTangent
	{
		vec2 uv;
		vec3 normal;
		vec3 tangent;
	};

	static D3D12_INPUT_ELEMENT_DESC input_layout_position[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static D3D12_INPUT_ELEMENT_DESC input_layout_position_uv[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static D3D12_INPUT_ELEMENT_DESC input_layout_position_normal[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static D3D12_INPUT_ELEMENT_DESC input_layout_position_uv_normal[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static D3D12_INPUT_ELEMENT_DESC input_layout_position_uv_normal_tangent[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	static D3D12_INPUT_ELEMENT_DESC input_layout_position_uv_normal_tangent_colors[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORDS", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}