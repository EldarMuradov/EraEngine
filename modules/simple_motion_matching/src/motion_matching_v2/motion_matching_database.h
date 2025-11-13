#pragma once 

#include "motion_matching_api.h"

#include "motion_matching/array.h"

#include <asset/game_asset.h>

#include <core/serialization/binary_serializer.h>

namespace era_engine 
{
	class KnnStructure;

	class ERA_MOTION_MATCHING_API MotionMatchingDatabase : public GameAsset
	{
	public:
		struct Sample
		{
			std::vector<float> features;

			uint32 anim_index = 0;
			float anim_position = 0.0f;

			ERA_BINARY_SERIALIZE(features, anim_index, anim_position)
		};

		struct DatabaseAsset
		{
			array1d<float> min_values;
			array1d<float> max_values;
			array1d<float> weights;

			float sample_rate = 10.0f;

			uint32 max_broardphase_candidates = 10;

			ERA_BINARY_SERIALIZE(min_values, max_values, weights, sample_rate, max_broardphase_candidates)
		};

		void build_structure();

		std::shared_ptr<Sample> search(const std::vector<float>& query) const;

		std::vector<float> normalize_query(const std::vector<float>& query) const;
		void subtract_column_means(array2d<float>& matrix) const;
		array2d<float> pack_query(const std::vector<float>& query) const;

		static std::string get_asset_type_impl();

		bool serialize(std::ostream& os) const override;
		bool deserialize(std::istream& is) override;

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