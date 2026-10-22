#pragma once

#include "physics_api.h"

#include "core/math.h"

#include <vector>

namespace era_engine::physics
{
    struct ERA_PHYSICS_API VehicleChassisConfig
    {
        float mass = 1500.0f;

        vec3 moi = vec3::zero;

        vec3 center_of_mass = vec3(0.0f, 0.45f, 0.0f);

        vec3 collision_center = vec3(0.0f, 0.8f, 0.0f);
        vec3 collision_half_extents = vec3(0.85f, 0.6f, 2.3f);

        std::vector<vec3> collision_hull;

        bool operator==(const VehicleChassisConfig&) const = default;
    };

    struct ERA_PHYSICS_API VehicleAxleConfig
    {
        vec3 position = vec3::zero;
        float track_width = 1.6f;

        float wheel_radius = 0.34f;
        float wheel_width = 0.25f;
        float wheel_mass = 20.0f;

        bool steered = false;
        bool driven = false;

        float brake_torque = 1500.0f;
        float handbrake_torque = 0.0f;

        float suspension_travel = 0.25f;
        float suspension_frequency = 1.6f;
        float suspension_damping_ratio = 0.4f;

        // Fraction of the axle spring stiffness.
        float anti_roll_ratio = 0.5f;

        float tire_lateral_stiffness = 20.0f;
        float tire_longitudinal_stiffness = 5.0f;
        float tire_grip = 1.0f;

        bool operator==(const VehicleAxleConfig&) const = default;
    };

    struct ERA_PHYSICS_API VehicleEngineConfig
    {
        float peak_torque = 350.0f;
        float idle_rpm = 900.0f;
        float max_rpm = 6500.0f;
        float moi = 1.0f;

        // Normalized torque at 0, 1/3, 2/3 and max rpm.
        vec4 torque_curve = vec4(0.8f, 1.0f, 1.0f, 0.85f);

        bool operator==(const VehicleEngineConfig&) const = default;
    };

    struct ERA_PHYSICS_API VehicleGearboxConfig
    {
        float reverse_ratio = 4.0f;
        std::vector<float> forward_ratios = {3.6f, 2.2f, 1.55f, 1.2f, 1.0f, 0.85f};
        float final_ratio = 3.6f;
        float switch_time = 0.25f;

        float shift_up_rpm = 0.75f;
        float shift_down_rpm = 0.4f;
        float shift_latency = 0.6f;

        float clutch_strength = 10.0f;

        bool operator==(const VehicleGearboxConfig&) const = default;
    };

    struct ERA_PHYSICS_API VehicleSteeringConfig
    {
        float max_angle = 35.0f;
        float high_speed_angle = 8.0f;
        float high_speed = 30.0f;
        float ackermann_strength = 1.0f;

        bool operator==(const VehicleSteeringConfig&) const = default;
    };

    struct ERA_PHYSICS_API VehicleControlConfig
    {
        // Rates in units per second.
        float throttle_rise = 4.0f;
        float throttle_fall = 6.0f;
        float brake_rise = 5.0f;
        float brake_fall = 8.0f;
        float steer_rate = 3.0f;
        float steer_return_rate = 5.0f;

        float reverse_speed = 0.8f;

        float coast_brake = 0.04f;

        float stability_assist = 0.5f;

        bool operator==(const VehicleControlConfig&) const = default;
    };

    struct ERA_PHYSICS_API VehicleConfig
    {
        VehicleChassisConfig chassis;
        std::vector<VehicleAxleConfig> axles;
        VehicleEngineConfig engine;
        VehicleGearboxConfig gearbox;
        VehicleSteeringConfig steering;
        VehicleControlConfig control;

        bool operator==(const VehicleConfig&) const = default;

        static VehicleConfig make_default();
    };
}
