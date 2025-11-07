#pragma once 

#include "motion_matching_api.h"

#include "motion_matching/array.h"

namespace era_engine 
{
	class KnnStructure;

	class ERA_MOTION_MATCHING_API MotionMatchingDatabase final
	{
	public:
		struct Sample
		{
			std::vector<float> features;

			uint32 anim_index = 0;
			float anim_position = 0.0f;
		};

		struct DatabaseAsset
		{
			array1d<float> min_values;
			array1d<float> max_values;
			array1d<float> weights;

			float sample_rate = 10.0f;

			uint32 max_broardphase_candidates = 10;
		};

		void build_structure();

		std::shared_ptr<Sample> search(const std::vector<float>& query) const;

		std::vector<float> normalize_query(const std::vector<float>& query) const;
		void subtract_column_means(array2d<float>& matrix) const;
		array2d<float> pack_query(const std::vector<float>& query) const;

	private:
		void fill_normalize_factors();

		std::shared_ptr<Sample> narrow_phase_search(const std::vector<std::shared_ptr<Sample>>& broadphase_candidates) const;

	public:
		std::shared_ptr<DatabaseAsset> asset;

		std::shared_ptr<KnnStructure> knn_structure;

		std::vector<std::shared_ptr<Sample>> samples;

		uint32 search_dimension = 0;

		array1d<float> transform_column_means;
		array2d<float> transform_matrix;

		std::vector<float> mean_values;
		std::vector<float> normalize_factors;
	};
}