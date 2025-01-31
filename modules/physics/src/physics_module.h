#pragma once

#include "physics_api.h"

#include <base/module.h>

namespace era_engine
{
	class ERA_PHYSICS_API PhysicsModule : public IModule
	{
	public:
		PhysicsModule() noexcept;

		~PhysicsModule() override;

		bool initialize(void* engine) override;
		bool terminate() override;

		RTTR_ENABLE(IModule)
	};
}