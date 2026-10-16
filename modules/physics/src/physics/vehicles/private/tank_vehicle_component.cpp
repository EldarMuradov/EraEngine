#include "physics/vehicles/tank_vehicle_component.h"

#include <ecs/serialization/component_registry.h>
#include <ecs/serialization/serialization_meta.h>

#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<TankVehicleComponent>("TankVehicleComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
			.constructor<>();
		ComponentRegistry::register_component<TankVehicleComponent>();
	}

	TankVehicleComponent::TankVehicleComponent(ref<Entity::EcsData> _data)
		: VehicleBaseComponent(_data)
	{
	}

}