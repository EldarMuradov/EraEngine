#include "motion_matching/motion_matching_component.h"
#include "motion_matching/motion_matching_database.h"
#include "motion_matching/features/motion_matching_feature_set.h"
#include "motion_matching/features/motion_matching_feature.h"

#include "motion_matching/features/pose_feature.h"
#include "motion_matching/features/phase_feature.h"
#include "motion_matching/features/trajectory_feature.h"

#include <animation/animation.h>

#include <rttr/registration>

namespace era_engine
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<MotionMatchingComponent>("MotionMatchingComponent")
			.constructor<ref<Entity::EcsData>>();
	}

	MotionMatchingComponent::MotionMatchingComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
		
	}

	MotionMatchingComponent::~MotionMatchingComponent()
	{
	}

	SearchResult MotionMatchingComponent::search_animation(const std::string& database_id, float dt) const
	{
		ref<MotionMatchingDatabase> database = MotionDatabaseRegistry::get_by_id(database_id);
		if (database == nullptr)
		{
			return SearchResult();
		}

		FeatureComputationContext context;
		context.fill_context(get_entity(), dt);

		PoseFeature pose_feature{};
		pose_feature.compute_features(context);

		TrajectoryFeature trajectory_feature{};
		trajectory_feature.compute_features(context);

		PhaseFeature phase_feature{};
		phase_feature.compute_features(context);

		MotionMatchingFeatureSet feature_set;

		feature_set.add_feature(pose_feature.get_type(), std::move(pose_feature.get_values()));
		feature_set.add_feature(trajectory_feature.get_type(), std::move(trajectory_feature.get_values()));
		feature_set.add_feature(phase_feature.get_type(), std::move(phase_feature.get_values()));

		return search_animation(feature_set, database_id);
	}

	SearchResult MotionMatchingComponent::search_animation(const MotionMatchingFeatureSet& feature_set, const std::string& database_id) const
	{
		using namespace animation;

		ref<MotionMatchingDatabase> database = MotionDatabaseRegistry::get_by_id(database_id);
		if (database == nullptr)
		{
			return SearchResult();
		}

		AnimationComponent* animation_component = get_entity().get_component<AnimationComponent>();

		SearchParams search_params;
		search_params.current_anim_position = animation_component->current_anim_position;
		search_params.current_animation = animation_component->current_animation;
		search_params.query = feature_set.get_all_feature_values();
		search_params.current_features = search_params.query;

		return database->search(search_params);
	}

}