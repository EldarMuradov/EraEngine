#pragma once

#include "motion_matching_api.h"

#include <ecs/reflection.h>
#include <ecs/entity.h>

namespace era_engine
{
	class ERA_MOTION_MATCHING_API MotionMatchingFeatureSet final
	{
	public:
		struct Feature
		{
			rttr::type feature_type = rttr::detail::get_invalid_type();
			std::vector<float> params_values;
		};

		MotionMatchingFeatureSet();

        void add_feature(rttr::type feature_type, std::vector<float>&& params_values);

        std::vector<rttr::type> get_feature_types() const;

        std::vector<float> get_all_feature_values() const;

        const std::vector<float>* get_feature_values(rttr::type type) const;

		void clear();

		template <class T>
		const std::vector<float>* get_feature_values() const
		{
			return get_feature_values(rttr::type::get<T>());
		}

	private:
		std::vector<Feature> features;
	};
}