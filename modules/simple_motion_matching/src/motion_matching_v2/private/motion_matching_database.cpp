#include "motion_matching_v2/motion_matching_database.h"
#include "motion_matching_v2/motion_matching_knn_structure.h"

#include "motion_matching_v2/hnsw/hnsw_knn_structure.h"

#include <asset/io.h>

#include <core/math.h>
#include <core/log.h>

namespace era_engine
{
	void MotionMatchingDatabase::fill_normalize_factors()
	{
		normalize_factors.reserve(max_values.size());
		for (size_t index = 0; index < max_values.size(); ++index)
		{
			if (fuzzy_equals(max_values[index], min_values[index], 1e-3f))
			{
				normalize_factors.push_back(0.0f);
			}
			else
			{
				normalize_factors.push_back(weights[index] / (max_values[index] - min_values[index]));
			}
		}
	}

    std::shared_ptr<MotionMatchingDatabase::Sample> MotionMatchingDatabase::narrow_phase_search(const std::vector<std::shared_ptr<Sample>>& broadphase_candidates) const
    {
        return broadphase_candidates.front();
    }

    void MotionMatchingDatabase::build_structure()
    {
        fill_normalize_factors();

        knn_structure->build_structure(*this);
    }

    std::shared_ptr<MotionMatchingDatabase::Sample> MotionMatchingDatabase::search(const std::vector<float>& query) const
    {
        std::vector<float> normalized_query_values = normalize_query(query);
        const array2d<float> packed_matrix_query = pack_query(normalized_query_values);

        float* packed_normalized_query = packed_matrix_query.data;
        const uint32 query_size = packed_matrix_query.cols * packed_matrix_query.rows;

        ASSERT(query_size == search_dimension);

        std::vector<std::shared_ptr<Sample>> knn = knn_structure->search_knn(packed_normalized_query, query_size, max_broardphase_candidates, *this);

        return narrow_phase_search(knn);
    }

    std::vector<float> MotionMatchingDatabase::normalize_query(const std::vector<float>& query) const
    {
        ASSERT(query.size() == mean_values.size());
        ASSERT(query.size() == normalize_factors.size());

        std::vector<float> result;
        result.reserve(query.size());
        for (size_t index = 0; index < query.size(); ++index)
        {
            if (fuzzy_equals(query[index], mean_values[index], 1e-3f))
            {
                result.push_back(0.0f);
            }
            else
            {
                result.push_back((query[index] - mean_values[index]) * normalize_factors[index]);
            }
        }

        return result;
    }

    void MotionMatchingDatabase::subtract_column_means(array2d<float>& matrix) const
    {
        const uint32 rows = matrix.rows;
        const uint32 columns = matrix.cols;

        for (uint32 i = 0; i < rows; ++i)
        {
            for (uint32 j = 0; j < columns; ++j)
            {
                matrix(i, j) = matrix(i, j) - transform_column_means[j];
            }
        }
    }

    array2d<float> MotionMatchingDatabase::pack_query(const std::vector<float>& query) const
    {
        array2d<float> pack_target(1, uint32(query.size()));
        pack_target.fill(query.data());

        subtract_column_means(pack_target);

        return transpose(transform_matrix * transpose(pack_target));
    }

    std::string MotionMatchingDatabase::get_asset_type_impl()
    {
        return std::string(".emmdb");
    }

    bool MotionMatchingDatabase::serialize(std::ostream& os) const
    {
        try
        {
            IO::write_value(os, knn_type);
            IO::write_value(os, search_dimension);
            IO::write_value(os, sample_rate);
            IO::write_value(os, max_broardphase_candidates);

            IO::write_vector(os, mean_values);
            IO::write_vector(os, normalize_factors);
            IO::write_vector(os, min_values);
            IO::write_vector(os, max_values);
            IO::write_vector(os, weights);
            IO::write_vector(os, transform_column_means);

            IO::write_value(os, transform_matrix.cols);
            IO::write_value(os, transform_matrix.rows);
            IO::write_data(os, transform_matrix.data, static_cast<uint32>(transform_matrix.cols * transform_matrix.rows));

            if (knn_structure->writable.empty())
            {
                knn_structure->build_structure(*this);
            }

            IO::write_value(os, knn_structure->writable.size());
            IO::write_data(os, knn_structure->writable.data(), knn_structure->writable.size());

            knn_structure->serialize(os, *this);
        }
        catch (...)
        {
            LOG_ERROR("Exception thrown while serializing asset!");
            return false;
        }

        return true;
    }

    bool MotionMatchingDatabase::deserialize(std::istream& is)
    {
        try
        {
            IO::read_value(is, knn_type);
            IO::read_value(is, search_dimension);
            IO::read_value(is, sample_rate);
            IO::read_value(is, max_broardphase_candidates);

            IO::read_vector(is, mean_values);
            IO::read_vector(is, normalize_factors);
            IO::read_vector(is, min_values);
            IO::read_vector(is, max_values);
            IO::read_vector(is, weights);
            IO::read_vector(is, transform_column_means);

            IO::read_value(is, transform_matrix.cols);
            IO::read_value(is, transform_matrix.rows);
            IO::read_data(is, transform_matrix.data, static_cast<uint32>(transform_matrix.cols * transform_matrix.rows));

            if (knn_type == KnnStructureType::HNSW)
            {
                knn_structure = ref<KnnStructure>(new HnswKnnStructure());
            }
            else
            {
                ASSERT(false);
            }

            uint32 knn_size = 0;
            IO::read_value(is, knn_size);

            std::vector<uint8> knn_data;
            knn_data.reserve(knn_size);
            IO::read_data(is, knn_data.data(), knn_size);

            knn_structure->writable.assign(
                reinterpret_cast<const char*>(knn_data.data()),
                knn_size);

            knn_structure->deserialize(is, *this);
        }
        catch (...)
        {
            LOG_ERROR("Exception thrown while deserializing asset!");
            return false;
        }

        return true;
    }
}
