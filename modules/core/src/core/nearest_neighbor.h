// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "core_api.h"

#include "core/math.h"

namespace era_engine
{
	struct ERA_CORE_API NearestNeighborQueryResult
	{
		uint32 index;
		float squared_distance;
	};

	class ERA_CORE_API PointCloud
	{
	public:
		PointCloud(vec3* _positions, uint32 _num_positions);
		~PointCloud();

		NearestNeighborQueryResult nearest_neighbor_index(vec3 query) const;

		inline size_t kdtree_get_point_count() const { return num_positions; }
		inline float kdtree_get_pt(const size_t idx, const size_t dim) const { return positions[idx].data[dim]; }
		template <typename BBOX> bool kdtree_get_bbox(BBOX& bb) const { return false; }

		uint32 num_positions = 0;

		vec3* positions = nullptr;
		void* index = nullptr;;
	};

}