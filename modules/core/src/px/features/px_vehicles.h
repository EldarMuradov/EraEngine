// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "px/core/px_physics_engine.h"
#include "core/project.h"
#include "core/string.h"

#if PX_VEHICLE

namespace era_engine
{
	struct transform_component;

	namespace physics 
	{
#if PX_VEHICLE
		using namespace physx;
		using namespace physx::vehicle2;
		using namespace snippetvehicle2;
#endif

		struct px_vehicle_base_component;

		void vehicleStep(px_vehicle_base_component* vehicle, transform_component* transform, float dt);

		void vehiclePostStep(px_vehicle_base_component* vehicle, float dt);
	}
}

#endif