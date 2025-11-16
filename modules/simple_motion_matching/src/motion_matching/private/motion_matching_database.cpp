#include "motion_matching/motion_matching_database.h"
#include "motion_matching/motion_matching_knn_structure.h"
#include "motion_matching/common.h"
#include "motion_matching/hnsw/hnsw_knn_structure.h"

#include <animation/animation_clip.h>

#include <asset/io.h>
#include <core/io/binary_data.h>
#include <core/io/binary_data_archive.h>

#include <core/math.h>
#include <core/log.h>

#include <core/traits.h>

namespace era_engine
{
    static float length(const std::vector<float>& query)
    {
        float sum = std::accumulate(query.begin(), query.end(), 0.0f, [](float sum, float v)
            {
                return sum + squaref(v);
            });

        return std::sqrt(sum);
    }

    static void normalize(std::vector<float>& query)
    {
        float l = length(query);
        if (fuzzy_equals(l, 0.0f))
        {
            return;
        }

        float normalizer = 1.0f / l;
        for (size_t index = 0; index < query.size(); ++index)
        {
            query[index] *= normalizer;
        }
    }

    static float square_distance(const std::vector<float>& lhs, const std::vector<float>& rhs)
    {
        ASSERT(lhs.size() == rhs.size());

        float result = 0.0f;
        for (size_t index = 0; index < lhs.size(); ++index)
        {
            const float diff = lhs[index] - rhs[index];
            result += squaref(diff);
        }

        return result;
    }

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

    SearchResult MotionMatchingDatabase::narrow_phase_search(const std::vector<std::shared_ptr<Sample>>& broadphase_candidates,
        const std::vector<float>& normalized_query,
        const SearchParams& params) const
    {
        bool has_current_frame_candidate = params.current_features.size() == normalized_query.size();

        std::optional<std::shared_ptr<MotionMatchingDatabase::Sample>> result_candidate;
        if (!broadphase_candidates.empty())
        {
            result_candidate = broadphase_candidates.front();
        }

        float result_candidate_to_query_distance_sqr = std::numeric_limits<float>::max();
        float current_candidate_to_query_distance_sqr = std::numeric_limits<float>::max();

        if (has_flag(narrow_phase_params.flags, NarrowPhaseFlags::EUCLIDIAN_DISTANCE_CHECK) ||
            has_flag(narrow_phase_params.flags, NarrowPhaseFlags::STABILITY_CHECK))
        {
            if (result_candidate.has_value())
            {
                result_candidate_to_query_distance_sqr = square_distance(normalized_query, normalize_query(result_candidate.value()->features));
            }

            if (has_current_frame_candidate)
            {
                const std::vector<float> current_frame_sample_normalized_features = normalize_query(params.current_features);
                current_candidate_to_query_distance_sqr = square_distance(normalized_query, current_frame_sample_normalized_features);
            }
        }

        if (has_flag(narrow_phase_params.flags, NarrowPhaseFlags::EUCLIDIAN_DISTANCE_CHECK))
        {
            const float max_euclidean_distance_sqr = squaref(narrow_phase_params.max_euclidean_distance);
            if (result_candidate.has_value() && result_candidate_to_query_distance_sqr >= max_euclidean_distance_sqr)
            {
                result_candidate.reset();
            }

            if (has_current_frame_candidate && current_candidate_to_query_distance_sqr >= max_euclidean_distance_sqr)
            {
                has_current_frame_candidate = false;
            }
        }

        if (!result_candidate.has_value())
        {
            if (has_current_frame_candidate)
            {
                return SearchResult(params.current_animation, database_id, params.current_features, params.current_anim_position);
            }
            else
            {
                return SearchResult();
            }
        }

        if (has_flag(narrow_phase_params.flags, NarrowPhaseFlags::SAME_FRAME_CHECK) &&
            params.current_animation == animations[result_candidate.value()->anim_index])
        {
            const float anim_position_diff = result_candidate.value()->anim_position - params.current_anim_position;

            if (abs(anim_position_diff) < narrow_phase_params.same_frame_time_threshold)
            {
                return SearchResult(params.current_animation, database_id, params.current_features, params.current_anim_position);

            }
        }

        if (has_flag(narrow_phase_params.flags, NarrowPhaseFlags::STABILITY_CHECK) &&
            has_current_frame_candidate)
        {
            const float current_frame_to_query_distance = std::sqrt(current_candidate_to_query_distance_sqr);
            const float result_candidate_to_query_distance = std::sqrt(result_candidate_to_query_distance_sqr);

            if (current_frame_to_query_distance - narrow_phase_params.stability_distance_threshold < result_candidate_to_query_distance)
            {
                return SearchResult(params.current_animation, database_id, params.current_features, params.current_anim_position);
            }
        }

        if (result_candidate.has_value())
        {
            ref<Sample> sample = result_candidate.value();
            return SearchResult(animations[sample->anim_index], database_id, sample->features, sample->anim_position);
        }
        else
        {
            return SearchResult();
        }
    }

    MotionMatchingDatabase::MotionMatchingDatabase(KnnStructureType _knn_type /*= KnnStructureType::DEFAULT*/)
    {
        knn_type = _knn_type;
        if (knn_type == KnnStructureType::HNSW)
        {
            knn_structure = ref<KnnStructure>(new HnswKnnStructure());
        }
        else
        {
            ASSERT(false);
        }
    }

    void MotionMatchingDatabase::build_structure()
    {
        fill_normalize_factors();

        knn_structure->build_structure(*this);
    }

    void MotionMatchingDatabase::bake()
    {
        for (uint32 i = 0; i < min_values.size(); i++)
        {
            min_values[i] = std::numeric_limits<float>::max();
            max_values[i] = std::numeric_limits<float>::min();
        }

        for (std::shared_ptr<Sample> motion_sample : samples)
        {
            ASSERT(motion_sample->features.size() == min_values.size());

            for (uint32 i = 0; i < min_values.size(); i++)
            {
                if (motion_sample->features[i] < min_values[i])
                {
                    min_values[i] = motion_sample->features[i];
                }

                if (motion_sample->features[i] > max_values[i])
                {
                    max_values[i] = motion_sample->features[i];
                }
            }
        }
    }

    SearchResult MotionMatchingDatabase::search(const SearchParams& params) const
    {
        std::vector<float> normalized_query_values = normalize_query(params.query);
        const array2d<float> packed_matrix_query = pack_query(normalized_query_values);

        float* packed_normalized_query = packed_matrix_query.data;
        const uint32 query_size = packed_matrix_query.cols * packed_matrix_query.rows;

        ASSERT(query_size == search_dimension);

        std::vector<std::shared_ptr<Sample>> knn = knn_structure->search_knn(packed_normalized_query, query_size, max_broardphase_candidates, *this);

        return narrow_phase_search(knn, normalized_query_values, params);
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
            // Base
            {
                IO::write_value(os, search_dimension);
                IO::write_value(os, sample_rate);
                IO::write_value(os, max_broardphase_candidates);
            }

            // Weights
            {
                IO::write_vector(os, normalize_factors);
                IO::write_vector(os, mean_values);
                IO::write_vector(os, min_values);
                IO::write_vector(os, max_values);
                IO::write_vector(os, weights);
            }

            // Animations
            {
                IO::write_vector(os, animations_asset_handles);
                IO::write_vector(os, animations_hashes);
            }

            // Features
            {
                IO::write_value(os, BinarySerializer::serialize(feature_types));
            }

            // Narrow phase
            {
                IO::write_value(os, narrow_phase_params.flags);
                IO::write_value(os, narrow_phase_params.same_frame_time_threshold);
                IO::write_value(os, narrow_phase_params.max_euclidean_distance);
                IO::write_value(os, narrow_phase_params.stability_distance_threshold);
            }

            // Transform
            {
                IO::write_vector(os, transform_column_means);
                IO::write_value(os, transform_matrix.cols);
                IO::write_value(os, transform_matrix.rows);
                IO::write_data(os, transform_matrix.data, static_cast<uint32>(transform_matrix.cols * transform_matrix.rows));
            }

            // Knn
            {
                IO::write_value(os, knn_type);

                if (knn_structure->writable.empty())
                {
                    knn_structure->build_structure(*this);
                }

                IO::write_value(os, knn_structure->writable.size());
                IO::write_data(os, knn_structure->writable.data(), knn_structure->writable.size());

                knn_structure->serialize(os, *this);
            }
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
            // Base
            {
                IO::read_value(is, search_dimension);
                IO::read_value(is, sample_rate);
                IO::read_value(is, max_broardphase_candidates);
            }

            // Weights
            {
                IO::read_vector(is, normalize_factors);
                IO::read_vector(is, mean_values);
                IO::read_vector(is, min_values);
                IO::read_vector(is, max_values);
                IO::read_vector(is, weights);
            }

            // Animations
            {
                IO::read_vector(is, animations_asset_handles);
                IO::read_vector(is, animations_hashes);

                GameAssetsProvider provider;

                for (const uint64 handle : animations_asset_handles)
                {
                    ref<animation::AnimationAssetClip> anim_clip = provider.load_game_asset_from_file<animation::AnimationAssetClip>(getPathFromAssetHandle(AssetHandle(handle)));
                    animations.emplace_back(anim_clip);
                }
            }

            // Features
            {
                BinaryDataArchive binary_feature_names;
                IO::read_value(is, binary_feature_names);

                std::vector<std::string> feature_serialized_names;
                BinarySerializer::deserialize(binary_feature_names.raw_data(), binary_feature_names.size(), feature_serialized_names);

                feature_types.reserve(feature_serialized_names.size());
                for (const std::string& param_name : feature_serialized_names)
                {
                    feature_types.emplace_back(param_name);
                }
            }

            // Narrow phase
            {
                IO::read_value(is, narrow_phase_params.flags);
                IO::read_value(is, narrow_phase_params.same_frame_time_threshold);
                IO::read_value(is, narrow_phase_params.max_euclidean_distance);
                IO::read_value(is, narrow_phase_params.stability_distance_threshold);
            }

            // Transform
            {
                IO::read_vector(is, transform_column_means);
                IO::read_value(is, transform_matrix.cols);
                IO::read_value(is, transform_matrix.rows);
                IO::read_data(is, transform_matrix.data, static_cast<uint32>(transform_matrix.cols * transform_matrix.rows));
            }

            // Knn
            {
                IO::read_value(is, knn_type);

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

            // Wait for loading
            for (const auto& animation : animations)
            {
                animation->load_job.wait_for_completion();
            }
        }
        catch (...)
        {
            LOG_ERROR("Exception thrown while deserializing asset!");
            return false;
        }

        return true;
    }
}
