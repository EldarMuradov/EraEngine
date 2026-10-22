#include "physics/vehicles/base_vehicle_component.h"
#include "physics/vehicles/tank_vehicle_component.h"
#include "physics/vehicles/w4_vehicle_component.h"

#include <ecs/serialization/component_registry.h>
#include <ecs/serialization/serialization_meta.h>

#include <rttr/registration>

namespace era_engine::physics
{

    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<VehicleBaseComponent>("VehicleBaseComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<>();
        ComponentRegistry::register_component<VehicleBaseComponent>();
    }

    VehicleBaseComponent::VehicleBaseComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
    }

    bool VehicleBaseComponent::is_created() const
    {
        return simulation != nullptr;
    }

    VehicleBaseComponent* VehicleUtils::get_vehicle_component(Entity entity)
    {
        if (W4VehicleComponent* w4_vehicle_component = entity.get_component_if_exists<W4VehicleComponent>())
        {
            return w4_vehicle_component;
        }
        else if (TankVehicleComponent* tank_vehicle_component = entity.get_component_if_exists<TankVehicleComponent>())
        {
            return tank_vehicle_component;
        }

        return nullptr;
    }
}
