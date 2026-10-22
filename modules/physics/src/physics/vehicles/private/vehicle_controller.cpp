#include "physics/vehicles/vehicle_controller.h"

#include <algorithm>
#include <cmath>

namespace era_engine::physics
{
    static constexpr float INPUT_THRESHOLD = 0.1f;
    static constexpr float CLUTCH_RELEASE_SPEED = 3.0f;

    static float approach_step(float current, float target, float rate, float dt)
    {
        const float max_delta = rate * dt;
        return current + std::clamp(target - current, -max_delta, max_delta);
    }

    VehicleCommands VehicleController::update(const VehicleDriverInput& input,
        const VehicleControlConfig& control,
        const VehicleSteeringConfig& steering,
        float forward_speed,
        float dt)
    {
        const float input_throttle = std::clamp(input.throttle, 0.0f, 1.0f);
        const float input_brake = std::clamp(input.brake, 0.0f, 1.0f);
        const float input_steer = std::clamp(input.steer, -1.0f, 1.0f);

        if (!reverse_mode)
        {
            reverse_mode = input_brake > INPUT_THRESHOLD && input_throttle < INPUT_THRESHOLD && forward_speed < control.reverse_speed;
        }
        else
        {
            reverse_mode = !(input_throttle > INPUT_THRESHOLD && forward_speed > -control.reverse_speed);
        }

        const float drive = reverse_mode ? input_brake : input_throttle;
        const float stop = reverse_mode ? input_throttle : input_brake;

        throttle = approach_step(throttle, drive, drive > throttle ? control.throttle_rise : control.throttle_fall, dt);
        brake = approach_step(brake, stop, stop > brake ? control.brake_rise : control.brake_fall, dt);

        const bool returning = input_steer * steer < 0.0f || std::abs(input_steer) < std::abs(steer);
        steer = approach_step(steer, input_steer, returning ? control.steer_return_rate : control.steer_rate, dt);

        const float speed_factor = steering.high_speed > 0.0f ? std::clamp(std::abs(forward_speed) / steering.high_speed, 0.0f, 1.0f) : 0.0f;
        const float min_steer_scale = steering.max_angle > 0.0f ? std::clamp(steering.high_speed_angle / steering.max_angle, 0.0f, 1.0f) : 1.0f;
        const float steer_scale = 1.0f + (min_steer_scale - 1.0f) * speed_factor;

        VehicleCommands commands;
        commands.throttle = throttle;
        commands.brake = (drive < INPUT_THRESHOLD && stop < INPUT_THRESHOLD) ? std::max(brake, control.coast_brake) : brake;
        commands.handbrake = input.handbrake ? 1.0f : 0.0f;
        commands.steer = steer * steer_scale;
        commands.reverse = reverse_mode;

        const bool idle_at_low_speed = throttle < INPUT_THRESHOLD && std::abs(forward_speed) < CLUTCH_RELEASE_SPEED;
        commands.clutch = (idle_at_low_speed || input.handbrake) ? 1.0f : 0.0f;

        return commands;
    }

    void VehicleController::reset()
    {
        *this = VehicleController();
    }
}
