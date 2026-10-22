#pragma once

#include "physics_api.h"

#include "physics/vehicles/vehicle_config.h"

namespace era_engine::physics
{
    struct VehicleDriverInput
    {
        float throttle = 0.0f; // [0, 1]
        float brake = 0.0f;    // [0, 1], reverse when nearly stopped.
        float steer = 0.0f;    // [-1, 1], positive = left.
        bool handbrake = false;
    };

    struct VehicleCommands
    {
        float throttle = 0.0f;
        float brake = 0.0f;
        float handbrake = 0.0f;
        float steer = 0.0f;  // Normalized, positive = left.
        float clutch = 0.0f; // 1 = disengaged.
        bool reverse = false;
    };

    class ERA_PHYSICS_API VehicleController
    {
    public:
        VehicleCommands update(const VehicleDriverInput& input,
            const VehicleControlConfig& control,
            const VehicleSteeringConfig& steering,
            float forward_speed,
            float dt);

        void reset();

        float throttle = 0.0f;
        float brake = 0.0f;
        float steer = 0.0f;
        bool reverse_mode = false;
    };
}
