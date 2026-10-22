#pragma once

#include "physics_api.h"

#include "physics/collision_types.h"

#include "ecs/component.h"
#include "ecs/entity.h"

namespace era_engine::physics
{
    class VehicleSimulation;

    class ERA_PHYSICS_API VehicleBaseComponent : public Component
    {
    public:
        static constexpr bool in_place_delete = true;

        VehicleBaseComponent() = default;
        VehicleBaseComponent(ref<Entity::EcsData> _data);

        bool is_created() const;

        CollisionType collision_type = CollisionType::VEHICLE;

        ERA_VIRTUAL_REFLECT(Component)

    protected:
        VehicleSimulation* simulation = nullptr;

        friend class VehicleSystem;
    };

    class ERA_PHYSICS_API VehicleUtils
    {
    public:
        VehicleUtils() = delete;

        static VehicleBaseComponent* get_vehicle_component(Entity entity);
    };
}
