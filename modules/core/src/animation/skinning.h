// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"

#include "geometry/mesh_builder.h"

namespace era_engine
{
	struct DXVertexBufferGroupView;
	struct DXVertexBufferView;
	struct ComputePass;
}

namespace era_engine::animation
{
	struct VertexRange
	{
		uint32 first_vertex;
		uint32 num_vertices;
	};

	ERA_CORE_API void initialize_skinning();
	ERA_CORE_API std::tuple<DXVertexBufferGroupView, mat4*> skin_object(const DXVertexBufferGroupView& vertex_buffer, VertexRange range, uint32 num_joints);
	ERA_CORE_API std::tuple<DXVertexBufferGroupView, mat4*> skin_object(const DXVertexBufferGroupView& vertex_buffer, uint32 num_vertices, uint32 num_joints);
	ERA_CORE_API std::tuple<DXVertexBufferGroupView, mat4*> skin_object(const DXVertexBufferGroupView& vertex_buffer, SubmeshInfo submesh, uint32 num_joints);

	ERA_CORE_API DXVertexBufferGroupView skin_cloth(const DXVertexBufferView& positions, uint32 grid_size_x, uint32 grid_size_y);

	ERA_CORE_API void perform_skinning(ComputePass* compute_pass);
}