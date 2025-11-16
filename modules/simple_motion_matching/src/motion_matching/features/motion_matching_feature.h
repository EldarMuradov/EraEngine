#pragma once

#include "motion_matching_api.h"

#include <ecs/reflection.h>
#include <ecs/entity.h>

namespace era_engine
{
	namespace animation
	{
		class SkeletonComponent;
		class AnimationComponent;
	}

	class TrajectoryComponent;
	class MotionComponent;

	struct ERA_MOTION_MATCHING_API FeatureComputationContext
	{
		void fill_context(Entity _entity, float _dt);

		World* world = nullptr;

		Entity entity = Entity::Null;

		animation::SkeletonComponent* skeleton_component = nullptr;
		animation::AnimationComponent* animation_component = nullptr;

		TrajectoryComponent* trajectory_component = nullptr;
		MotionComponent* motion_component = nullptr;

		float dt = 0.0f;
	};

	class ERA_MOTION_MATCHING_API MotionMatchingFeature
	{
	public:
		MotionMatchingFeature();
		virtual ~MotionMatchingFeature();

		const std::vector<float>& get_values() const;

		virtual void compute_features(const FeatureComputationContext& context);
		void store_features(std::vector<float>&& _values);

		ERA_REFLECT

	protected:
		std::vector<float> values;
	};
}