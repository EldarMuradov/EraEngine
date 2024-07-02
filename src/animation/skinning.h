// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"

#include "geometry/mesh_builder.h"

namespace era_engine
{
	struct dx_vertex_buffer_group_view;
	struct dx_vertex_buffer_view;
	struct compute_pass;
}

namespace era_engine::animation
{
	struct vertex_range
	{
		uint32 firstVertex;
		uint32 numVertices;
	};

	void initializeSkinning();
	NODISCARD std::tuple<dx_vertex_buffer_group_view, mat4*> skinObject(const dx_vertex_buffer_group_view& vertexBuffer, vertex_range range, uint32 numJoints);
	NODISCARD std::tuple<dx_vertex_buffer_group_view, mat4*> skinObject(const dx_vertex_buffer_group_view& vertexBuffer, uint32 numVertices, uint32 numJoints);
	NODISCARD std::tuple<dx_vertex_buffer_group_view, mat4*> skinObject(const dx_vertex_buffer_group_view& vertexBuffer, submesh_info submesh, uint32 numJoints);

	NODISCARD dx_vertex_buffer_group_view skinCloth(const dx_vertex_buffer_view& positions, uint32 gridSizeX, uint32 gridSizeY);

	void performSkinning(compute_pass* computePass);
}