#include "physics/vehicles/w4_vehicle_component.h"

#include <ecs/serialization/component_registry.h>
#include <ecs/serialization/serialization_meta.h>

#include <rttr/registration>

namespace era_engine::physics
{
    RTTR_REGISTRATION
    {
        using namespace rttr;
        registration::class_<W4VehicleComponent>("W4VehicleComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("config", &W4VehicleComponent::config);
        ComponentRegistry::register_component<W4VehicleComponent>();

        registration::class_<VehicleWheelComponent>("VehicleWheelComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("wheel_id", &VehicleWheelComponent::wheel_id)
            .property("should_spin", &VehicleWheelComponent::should_spin);
        ComponentRegistry::register_component<VehicleWheelComponent>();

        registration::class_<VehicleDoorComponent>("VehicleDoorComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("open", &VehicleDoorComponent::open)
            .property("max_angle", &VehicleDoorComponent::max_angle)
            .property("speed", &VehicleDoorComponent::speed)
            .property("collision_hull", &VehicleDoorComponent::collision_hull);
        ComponentRegistry::register_component<VehicleDoorComponent>();

        registration::class_<VehicleShapeComponent>("VehicleShapeComponent")(rttr::metadata(SerializationMeta::NOT_SERIALIZABLE_CLASS, true))
            .constructor<ref<Entity::EcsData>>();
        ComponentRegistry::register_component<VehicleShapeComponent>();

        registration::class_<VehicleSteeringWheelComponent>("VehicleSteeringWheelComponent")
            .constructor<ref<Entity::EcsData>>()
            .property("axis", &VehicleSteeringWheelComponent::axis)
            .property("ratio", &VehicleSteeringWheelComponent::ratio);
        ComponentRegistry::register_component<VehicleSteeringWheelComponent>();
    }

    W4VehicleComponent::W4VehicleComponent(ref<Entity::EcsData> _data)
        : VehicleBaseComponent(_data)
    {
    }

    const VehicleDebugData& W4VehicleComponent::get_debug_data() const
    {
        return debug_data;
    }

    VehicleDoorComponent::VehicleDoorComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
    }

    VehicleShapeComponent::VehicleShapeComponent(ref<Entity::EcsData> _data)
        : ShapeComponent(_data)
    {
    }

    VehicleSteeringWheelComponent::VehicleSteeringWheelComponent(ref<Entity::EcsData> _data)
        : Component(_data)
    {
    }

    void VehicleDoorComponent::toggle()
    {
        open = open > 0.5f ? 0.0f : 1.0f;
    }

    VehicleWheelComponent::VehicleWheelComponent(ref<Entity::EcsData> _data, uint32 _wheel_id)
        : Component(_data), wheel_id(_wheel_id)
    {
    }
}
