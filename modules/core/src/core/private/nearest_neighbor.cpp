// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "core/nearest_neighbor.h"

#include <nanoflann/nanoflann.hpp>

namespace era_engine
{
    PointCloud::PointCloud(vec3* _positions, uint32 _num_positions)
    {
        using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<float, PointCloud>,
            PointCloud, 3>;

        positions = _positions;
        num_positions = _num_positions;

        index = new KDTree(3, *this, {10});
    }

    PointCloud::~PointCloud()
    {
        using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<float, PointCloud>,
            PointCloud, 3>;

        KDTree* _index = (KDTree*)index;

        delete _index;
    }

    NearestNeighborQueryResult PointCloud::nearest_neighbor_index(vec3 query) const
    {
        using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
            nanoflann::L2_Simple_Adaptor<float, PointCloud>,
            PointCloud, 3>;

        KDTree* _index = (KDTree*)index;

        size_t result_index = 0;
        float squared_distance = 0.0f;
        nanoflann::KNNResultSet<float> result_set(1);
        result_set.init(&result_index, &squared_distance);

        _index->findNeighbors(result_set, &query.data[0]);

        return {(uint32)result_index, squared_distance};
    }
}