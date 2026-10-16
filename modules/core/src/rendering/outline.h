// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core/math.h"

#include "geometry/mesh.h"

namespace era_engine
{
	void initialize_outline_pipelines();
	void render_outline(struct LDRRenderPass* render_pass, const mat4& transform, DXVertexBufferView vertex_buffer, DXIndexBufferView index_buffer, SubmeshInfo submesh);
	void render_outline(struct LDRRenderPass* render_pass, const mat4& transform, DXVertexBufferGroupView vertex_buffer, DXIndexBufferView index_buffer, SubmeshInfo submesh);
}