#pragma once

#include "core_api.h"

#include <base/module.h>

namespace era_engine
{
	class ERA_CORE_API CoreModule : public IModule
	{
	public:
		CoreModule() noexcept;
		~CoreModule() override;

		bool initialize(void* engine) override;
		bool terminate() override;

		RTTR_ENABLE(IModule)
	};
}