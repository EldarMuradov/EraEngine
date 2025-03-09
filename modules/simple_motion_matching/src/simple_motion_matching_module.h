#pragma once

#include "motion_matching_api.h"

#include <base/module.h>

namespace era_engine
{
	class ERA_MOTION_MATCHING_API SimpleMotionMatchingModule : public IModule
	{
	public:
		SimpleMotionMatchingModule() noexcept;

		~SimpleMotionMatchingModule() override;

		bool initialize(void* engine) override;
		bool terminate() override;

		RTTR_ENABLE(IModule)
	};
}