#pragma once

#include "motion_matching_api.h"

#include "motion_matching/features/motion_matching_feature.h"

#include <animation/skeleton.h>

#include <ecs/reflection.h>

namespace era_engine
{
    class ERA_MOTION_MATCHING_API PhaseFeatureDesc : public FeatureDesc
    {
    public:
        PhaseFeatureDesc() = default;
        ~PhaseFeatureDesc() override = default;

        uint32 joint_id = INVALID_JOINT;

        float contact_height_threshold = 0.08f;
        float contact_speed_threshold = 0.8f;

        float full_amplitude_speed = 0.8f;

        ERA_VIRTUAL_BINARY_SERIALIZE(FeatureDesc::name, joint_id, contact_height_threshold, contact_speed_threshold, full_amplitude_speed);

        ERA_VIRTUAL_REFLECT(FeatureDesc)
    };

    class ERA_MOTION_MATCHING_API PhaseFeature : public MotionMatchingFeature
    {
    public:
        PhaseFeature() = default;
        PhaseFeature(const PhaseFeature&) = default;
        ~PhaseFeature() override;

        uint32 get_feature_size() const override;

        std::vector<float> compute_features(const FeatureComputationContext& context) const override;

        bool mark_animation(const animation::SkeletonComponent* skeleton_component, ref<animation::AnimationAssetClip> clip) const override;
        bool sample_animation(const animation::Skeleton* skeleton, ref<animation::AnimationAssetClip> clip, float sample_rate, std::vector<ref<MotionMatchingDatabase::Sample>>& out_samples) const override;

        ERA_VIRTUAL_REFLECT(MotionMatchingFeature)
    };
}
