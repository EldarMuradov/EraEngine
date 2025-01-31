// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/nearest_neighbor.h"

#include <nanoflann/nanoflann.hpp>

namespace era_engine
{
    PointCloud::PointCloud(vec3* _positions, uint32 _num_positions)
    {
        using kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<float, PointCloud>,
            PointCloud, 3>;

        positions = _positions;
        num_positions = _num_positions;

        index = new kd_tree_t(3, *this, { 10 });
    }

    PointCloud::~PointCloud()
    {
        using kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<float, PointCloud>,
            PointCloud, 3>;

        kd_tree_t* _index = (kd_tree_t*)index;

        delete _index;
    }

    NearestNeighborQueryResult PointCloud::nearest_neighbor_index(vec3 query) const
    {
        using kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<float, PointCloud>,
            PointCloud, 3>;

        kd_tree_t* _index = (kd_tree_t*)index;

        size_t result_index = 0;
        float squared_distance = 0.0f;
        nanoflann::KNNResultSet<float> result_set(1);
        result_set.init(&result_index, &squared_distance);

        _index->findNeighbors(result_set, &query.data[0]);

        return { (uint32)result_index, squared_distance };
    }
}