#pragma once 

#include "motion_matching_api.h"

#include "motion_matching/array.h"

#include <asset/game_asset.h>

#include <animation/animation_clip.h>

#include <core/serialization/binary_serializer.h>

namespace era_engine 
{
	class KnnStructure;

	enum KnnStructureType : uint8
	{
		HNSW = 0,
		DEFAULT = HNSW,
		COUNT
	};

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
		std::vector<std::shared_ptr<Sample>> samples;

		std::shared_ptr<KnnStructure> knn_structure;
		KnnStructureType knn_type = KnnStructureType::DEFAULT;

		uint32 search_dimension = 0;
		float sample_rate = 10.0f;
		uint32 max_broardphase_candidates = 10;

		std::vector<float> mean_values;
		std::vector<float> normalize_factors;

		std::vector<float> weights;
		std::vector<float> min_values;
		std::vector<float> max_values;

		std::vector<float> transform_column_means;
		array2d<float> transform_matrix;

		std::vector<ref<AnimationAssetClip>> animations;
	};
}