// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/features/px_vehicles.h"
#include "px/features/px_vehicle_component.h"

#include "scene/components.h"

namespace era_engine::physics
{
#if PX_VEHICLE

	void vehicleStep(px_vehicle_base_component* vehicle, transform_component* transform, float dt)
	{
		vehicle->simulationStep(dt);

		trs pose = vehicle->getTransform();
		transform->position = pose.position;
	}

	void vehiclePostStep(px_vehicle_base_component* vehicle, float dt)
	{
		vehicle->simulationPostStep(dt);
	}

#endif

}

