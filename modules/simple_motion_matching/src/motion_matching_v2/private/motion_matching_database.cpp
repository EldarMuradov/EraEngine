#include "motion_matching_v2/motion_matching_database.h"
#include "motion_matching_v2/motion_matching_knn_structure.h"

#include <core/math.h>

namespace era_engine
{
	void MotionMatchingDatabase::fill_normalize_factors()
	{
		normalize_factors.reserve(asset->max_values.size);
		for (size_t index = 0; index < asset->max_values.size; ++index)
		{
			if (fuzzy_equals(asset->max_values(index), asset->min_values(index), 1e-3f))
			{
				normalize_factors.push_back(0.0f);
			}
			else
			{
				normalize_factors.push_back(asset->weights(index) / (asset->max_values(index) - asset->min_values(index)));
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

        std::vector<std::shared_ptr<Sample>> knn = knn_structure->search_knn(packed_normalized_query, query_size, asset->max_broardphase_candidates, *this);

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
                matrix(i, j) = matrix(i, j) - transform_column_means(j);
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
}
