#pragma once 

#include "motion_matching_api.h"

#include "motion_matching/array.h"

#include <asset/game_asset.h>

#include <core/serialization/binary_serializer.h>

namespace era_engine 
{
	namespace animation
	{
		class AnimationAssetClip;
	}

	class KnnStructure;

	enum class KnnStructureType : uint8
	{
		HNSW = 0,
		DEFAULT = HNSW,
		COUNT
	};

	enum class NarrowPhaseFlags : uint8
	{
		NONE = 0,
		SAME_FRAME_CHECK = 1,
		EUCLIDIAN_DISTANCE_CHECK = 2,
		STABILITY_CHECK = 4,

		ALL = SAME_FRAME_CHECK | EUCLIDIAN_DISTANCE_CHECK | STABILITY_CHECK
	};
	DEFINE_BITWISE_OPERATORS_FOR_ENUM(NarrowPhaseFlags);

	struct ERA_MOTION_MATCHING_API NarrowPhaseParams final
	{
		NarrowPhaseFlags flags = NarrowPhaseFlags::NONE;
		float same_frame_time_threshold = 0.5f;
		float max_euclidean_distance = 15.0f;
		float stability_distance_threshold = 0.1f;
	};

	struct ERA_MOTION_MATCHING_API SearchParams final
	{
		ref<animation::AnimationAssetClip> current_animation;
		std::vector<float> current_features;
		std::vector<float> query;

		float current_anim_position = 0.0f;
	};

	struct ERA_MOTION_MATCHING_API SearchResult final
	{
		ref<animation::AnimationAssetClip> animation;
		std::string database_id;

		std::vector<float> found_features;
		float anim_position = 0.0f;
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

		MotionMatchingDatabase(KnnStructureType _knn_type = KnnStructureType::DEFAULT);
		void build_structure();

		void bake();

		SearchResult search(const SearchParams& params) const;

		std::vector<float> normalize_query(const std::vector<float>& query) const;
		void subtract_column_means(array2d<float>& matrix) const;
		array2d<float> pack_query(const std::vector<float>& query) const;

		static std::string get_asset_type_impl();

		bool serialize(std::ostream& os) const override;
		bool deserialize(std::istream& is) override;

	protected:
		void fill_normalize_factors();

		SearchResult narrow_phase_search(const std::vector<std::shared_ptr<Sample>>& broadphase_candidates,
			const std::vector<float>& normalized_query,
			const SearchParams& params) const;

	public:
		std::vector<std::shared_ptr<Sample>> samples;

		std::shared_ptr<KnnStructure> knn_structure;
		KnnStructureType knn_type = KnnStructureType::DEFAULT;

		uint32 search_dimension = 0;
		float sample_rate = 10.0f;
		uint32 max_broardphase_candidates = 10;

		NarrowPhaseParams narrow_phase_params;
		
		std::string database_id;
		std::vector<std::string> feature_types;

		std::vector<float> mean_values;
		std::vector<float> normalize_factors;

		std::vector<float> weights;
		std::vector<float> min_values;
		std::vector<float> max_values;

		std::vector<float> transform_column_means;
		array2d<float> transform_matrix;

		std::vector<uint64> animations_asset_handles;
		std::vector<uint32> animations_hashes;
		std::vector<ref<animation::AnimationAssetClip>> animations;
	};
}