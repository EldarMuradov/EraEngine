#include "motion_matching/hnsw/hnsw_knn_structure.h"

#include <core/math.h>

namespace era_engine
{
    HnswKnnStructure::~HnswKnnStructure()
    {
    }

    void HnswKnnStructure::rebuild_structure(const MotionMatchingDatabase& database)
    {
        MatrixX features_matrix;
        database.build_feature_matrix(features_matrix);
        database.substract_column_means(features_matrix);

        MatrixX compressed_matrix;
        database.project_features(features_matrix, compressed_matrix);

        build_structure_from_matrix(database, compressed_matrix);
    }

    void HnswKnnStructure::build_structure_from_matrix(const MotionMatchingDatabase& database, const MatrixX& compressed_matrix)
    {
        ASSERT(compressed_matrix.rows() == database.samples.size() && compressed_matrix.cols() == database.search_dimension);

        hnsw_l2_space = hnswlib::L2Space(database.search_dimension);

        hnsw = std::shared_ptr<hnswlib::HierarchicalNSW<float>>(new hnswlib::HierarchicalNSW<float>(&hnsw_l2_space, database.samples.size(), max_edges_per_vertex, construction_exploration_factor));

        for (uint32 point_index = 0; point_index < compressed_matrix.rows(); ++point_index)
        {
            hnsw->addPoint(compressed_matrix.row(point_index), point_index);
        }

        hnsw->setEf(search_exploration_factor);

        std::stringstream hnsw_stream;
        hnsw->saveIndex(hnsw_stream);

        writable = hnsw_stream.str();
    }

    std::vector<std::shared_ptr<MotionMatchingDatabase::Sample>> HnswKnnStructure::search_knn(float* query, uint32 query_size, uint32 max_candidates, const MotionMatchingDatabase& database)
    {
        std::vector<std::shared_ptr<MotionMatchingDatabase::Sample>> found_samples;

        std::priority_queue<std::pair<float, hnswlib::labeltype>> search_result = hnsw->searchKnn(query, max_candidates);

        const int32 size = int32(search_result.size());
        found_samples.resize(size);

        for (int32 index = size - 1; index >= 0; --index)
        {
            const int32 sample_index = int32(search_result.top().second);
            search_result.pop();

            ASSERT(sample_index < database.samples.size());
            found_samples[index] = database.samples[sample_index];
        }

        return found_samples;
    }

	bool HnswKnnStructure::serialize(std::ostream& os, const MotionMatchingDatabase& database) const
	{
        IO::write_value(os, max_edges_per_vertex);
        IO::write_value(os, construction_exploration_factor);
        IO::write_value(os, search_exploration_factor);

		return true;
	}

    bool HnswKnnStructure::deserialize(std::istream& is, const MotionMatchingDatabase& database)
    {
        IO::read_value(is, max_edges_per_vertex);
        IO::read_value(is, construction_exploration_factor);
        IO::read_value(is, search_exploration_factor);

        hnsw_l2_space = hnswlib::L2Space(database.search_dimension);

        if (writable.empty())
        {
            rebuild_structure(database);
        }
        else
        {
            hnsw = std::shared_ptr<hnswlib::HierarchicalNSW<float>>(new hnswlib::HierarchicalNSW<float>(&hnsw_l2_space));

            std::stringbuf string_buf(writable);
            std::istream hnsw_stream(&string_buf);

            hnsw->loadIndex(hnsw_stream, &hnsw_l2_space);
            hnsw->setEf(search_exploration_factor);
        }
        return hnsw != nullptr;
    }
}
