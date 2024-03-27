// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "math.h"

struct nearest_neighbor_query_result
{
	uint32 index;
	float squaredDistance;
};

struct point_cloud
{
	vec3* positions;
	uint32 numPositions;

	void* index;

	point_cloud(vec3* positions, uint32 numPositions);
	~point_cloud();

	NODISCARD nearest_neighbor_query_result nearestNeighborIndex(vec3 query);

	inline NODISCARD size_t kdtree_get_point_count() const { return numPositions; }
	inline NODISCARD float kdtree_get_pt(const size_t idx, const size_t dim) const { return positions[idx].data[dim]; }
	template <typename BBOX> NODISCARD bool kdtree_get_bbox(BBOX& bb) const { return false; }
};
