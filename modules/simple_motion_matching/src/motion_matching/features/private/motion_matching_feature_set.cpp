#include "motion_matching/features/motion_matching_feature_set.h"

namespace era_engine
{
	MotionMatchingFeatureSet::MotionMatchingFeatureSet()
	{
	}

	void MotionMatchingFeatureSet::add_feature(rttr::type feature_type, std::vector<float>&& params_values)
    {
        const auto iter = std::find_if(features.begin(), features.end(), [feature_type](const Feature& feature)
            {
                return feature.feature_type == feature_type;
            });

        if (iter != features.end())
        {
            iter->params_values = std::move(params_values);
        }
        else
        {
            Feature& feature = features.emplace_back();
            feature.feature_type = feature_type;
            feature.params_values = std::move(params_values);
        }
    }

    std::vector<rttr::type> MotionMatchingFeatureSet::get_feature_types() const
    {
        std::vector<rttr::type> result;
        for (const Feature& feature_node : features)
        {
            result.push_back(feature_node.feature_type);
        }

        return result;
    }

    std::vector<float> MotionMatchingFeatureSet::get_all_feature_values() const
    {
        std::vector<float> result;
        for (const Feature& feature_node : features)
        {
            result.insert(result.end(), feature_node.params_values.begin(), feature_node.params_values.end());
        }

        return result;
    }

    const std::vector<float>* MotionMatchingFeatureSet::get_feature_values(rttr::type type) const
    {
        const auto iter = std::find_if(features.begin(), features.end(), [type](const Feature& feature)
            {
                return feature.feature_type == type;
            });

        if (iter != features.end())
        {
            return &iter->params_values;
        }

        return nullptr;
    }

    void MotionMatchingFeatureSet::clear()
    {
        features.clear();
    }
}