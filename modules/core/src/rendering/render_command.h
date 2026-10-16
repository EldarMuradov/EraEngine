// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "geometry/mesh.h"

#include "rendering/shadow_map_cache.h"

namespace era_engine
{
	struct ParticleDrawInfo
	{
		ref<DXBuffer> particle_buffer;
		ref<DXBuffer> alive_list;
		ref<DXBuffer> command_buffer;
		uint32 alive_list_offset;
		uint32 command_buffer_offset;
		uint32 root_parameter_offset;
	};

	template <typename render_data_t>
	struct ParticleRenderCommand
	{
		DXVertexBufferGroupView vertex_buffer;
		DXIndexBufferView index_buffer;
		ParticleDrawInfo draw_info;

		render_data_t data;
	};
}