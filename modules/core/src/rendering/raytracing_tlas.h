// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "rendering/raytracing.h"

namespace era_engine
{
	struct RaytracingInstanceHandle
	{
		uint32 instance_index;
	};

	struct RaytracingTLAS
	{
		void initialize(RaytracingASRebuildMode rebuild_mode = RAYTRACING_AS_REBUILD);

		// Call these each frame to rebuild the structure
		void reset();
		RaytracingInstanceHandle instantiate(RaytracingObjectType type, const trs& transform);
		void build(struct DXCommandList* cl);

		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> all_instances;

		RaytracingASRebuildMode rebuild_mode;

		ref<DXBuffer> scratch;
		ref<DXBuffer> tlas;
	};
}