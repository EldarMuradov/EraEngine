#pragma once

#include "physics_api.h"

#include "physics/vehicles/base_vehicle_component.h"
#include "physics/vehicles/vehicle_config.h"
#include "physics/vehicles/vehicle_controller.h"
#include "physics/shape_component.h"

namespace era_engine::physics
{
    struct VehicleDebugData
    {
        float forward_speed = 0.0f; // m/s
        float engine_rpm = 0.0f;
        int32 gear = 0; // -1 = reverse, 0 = neutral.
        uint32 grounded_wheels = 0;
        float steer_angle = 0.0f; // rad
        VehicleCommands commands;
    };

    class ERA_PHYSICS_API W4VehicleComponent : public VehicleBaseComponent
    {
    public:
        W4VehicleComponent() = default;
        W4VehicleComponent(ref<Entity::EcsData> _data);

        const VehicleDebugData& get_debug_data() const;

        VehicleConfig config = VehicleConfig::make_default();

        VehicleDriverInput driver_input;

        ERA_VIRTUAL_REFLECT(VehicleBaseComponent)

    private:
        VehicleController controller;
        VehicleDebugData debug_data;

        VehicleConfig built_config;
        bool build_failed = false;

        uint32 generation = 0;

        friend class VehicleSystem;
    };

    class ERA_PHYSICS_API VehicleWheelComponent : public Component
    {
    public:
        VehicleWheelComponent() = default;
        VehicleWheelComponent(ref<Entity::EcsData> _data, uint32 _wheel_id = 0);

        // axle * 2 + (left ? 1 : 0)
        uint32 wheel_id = 0;

        bool should_spin = true;

        ERA_VIRTUAL_REFLECT(Component)
    };

    class ERA_PHYSICS_API VehicleDoorComponent : public Component
    {
    public:
        VehicleDoorComponent() = default;
        VehicleDoorComponent(ref<Entity::EcsData> _data);

        void toggle();

        float open = 0.0f;       // [0, 1].
        float max_angle = 70.0f; // deg
        float speed = 1.5f;

        std::vector<vec3> collision_hull;

        ERA_VIRTUAL_REFLECT(Component)

    private:
        float current = 0.0f;

        physx::PxShape* shape = nullptr;
        uint32 shape_generation = 0;

        friend class VehicleSystem;
    };

    class ERA_PHYSICS_API VehicleShapeComponent final : public ShapeComponent
    {
    public:
        VehicleShapeComponent() = default;
        VehicleShapeComponent(ref<Entity::EcsData> _data);

        ERA_VIRTUAL_REFLECT(ShapeComponent)

    private:
        friend class VehicleSystem;
    };

    class ERA_PHYSICS_API VehicleSteeringWheelComponent : public Component
    {
    public:
        VehicleSteeringWheelComponent() = default;
        VehicleSteeringWheelComponent(ref<Entity::EcsData> _data);

        vec3 axis = vec3(0.0f, 0.0f, -1.0f);
        float ratio = 14.0f; // deg

        ERA_VIRTUAL_REFLECT(Component)
    };
}
