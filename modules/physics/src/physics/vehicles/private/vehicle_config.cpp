#include "physics/vehicles/vehicle_config.h"

#include <rttr/registration>

namespace era_engine::physics
{
    RTTR_REGISTRATION
    {
        using namespace rttr;

        registration::class_<VehicleChassisConfig>("VehicleChassisConfig")
            .constructor<>()
            .property("mass", &VehicleChassisConfig::mass)
            .property("moi", &VehicleChassisConfig::moi)
            .property("center_of_mass", &VehicleChassisConfig::center_of_mass)
            .property("collision_center", &VehicleChassisConfig::collision_center)
            .property("collision_half_extents", &VehicleChassisConfig::collision_half_extents)
            .property("collision_hull", &VehicleChassisConfig::collision_hull);

        registration::class_<VehicleAxleConfig>("VehicleAxleConfig")
            .constructor<>()
            .property("position", &VehicleAxleConfig::position)
            .property("track_width", &VehicleAxleConfig::track_width)
            .property("wheel_radius", &VehicleAxleConfig::wheel_radius)
            .property("wheel_width", &VehicleAxleConfig::wheel_width)
            .property("wheel_mass", &VehicleAxleConfig::wheel_mass)
            .property("steered", &VehicleAxleConfig::steered)
            .property("driven", &VehicleAxleConfig::driven)
            .property("brake_torque", &VehicleAxleConfig::brake_torque)
            .property("handbrake_torque", &VehicleAxleConfig::handbrake_torque)
            .property("suspension_travel", &VehicleAxleConfig::suspension_travel)
            .property("suspension_frequency", &VehicleAxleConfig::suspension_frequency)
            .property("suspension_damping_ratio", &VehicleAxleConfig::suspension_damping_ratio)
            .property("anti_roll_ratio", &VehicleAxleConfig::anti_roll_ratio)
            .property("tire_lateral_stiffness", &VehicleAxleConfig::tire_lateral_stiffness)
            .property("tire_longitudinal_stiffness", &VehicleAxleConfig::tire_longitudinal_stiffness)
            .property("tire_grip", &VehicleAxleConfig::tire_grip);

        registration::class_<VehicleEngineConfig>("VehicleEngineConfig")
            .constructor<>()
            .property("peak_torque", &VehicleEngineConfig::peak_torque)
            .property("idle_rpm", &VehicleEngineConfig::idle_rpm)
            .property("max_rpm", &VehicleEngineConfig::max_rpm)
            .property("moi", &VehicleEngineConfig::moi)
            .property("torque_curve", &VehicleEngineConfig::torque_curve);

        registration::class_<VehicleGearboxConfig>("VehicleGearboxConfig")
            .constructor<>()
            .property("reverse_ratio", &VehicleGearboxConfig::reverse_ratio)
            .property("forward_ratios", &VehicleGearboxConfig::forward_ratios)
            .property("final_ratio", &VehicleGearboxConfig::final_ratio)
            .property("switch_time", &VehicleGearboxConfig::switch_time)
            .property("shift_up_rpm", &VehicleGearboxConfig::shift_up_rpm)
            .property("shift_down_rpm", &VehicleGearboxConfig::shift_down_rpm)
            .property("shift_latency", &VehicleGearboxConfig::shift_latency)
            .property("clutch_strength", &VehicleGearboxConfig::clutch_strength);

        registration::class_<VehicleSteeringConfig>("VehicleSteeringConfig")
            .constructor<>()
            .property("max_angle", &VehicleSteeringConfig::max_angle)
            .property("high_speed_angle", &VehicleSteeringConfig::high_speed_angle)
            .property("high_speed", &VehicleSteeringConfig::high_speed)
            .property("ackermann_strength", &VehicleSteeringConfig::ackermann_strength);

        registration::class_<VehicleControlConfig>("VehicleControlConfig")
            .constructor<>()
            .property("throttle_rise", &VehicleControlConfig::throttle_rise)
            .property("throttle_fall", &VehicleControlConfig::throttle_fall)
            .property("brake_rise", &VehicleControlConfig::brake_rise)
            .property("brake_fall", &VehicleControlConfig::brake_fall)
            .property("steer_rate", &VehicleControlConfig::steer_rate)
            .property("steer_return_rate", &VehicleControlConfig::steer_return_rate)
            .property("reverse_speed", &VehicleControlConfig::reverse_speed)
            .property("coast_brake", &VehicleControlConfig::coast_brake)
            .property("stability_assist", &VehicleControlConfig::stability_assist);

        registration::class_<VehicleConfig>("VehicleConfig")
            .constructor<>()
            .property("chassis", &VehicleConfig::chassis)
            .property("axles", &VehicleConfig::axles)
            .property("engine", &VehicleConfig::engine)
            .property("gearbox", &VehicleConfig::gearbox)
            .property("steering", &VehicleConfig::steering)
            .property("control", &VehicleConfig::control);
    }

    VehicleConfig VehicleConfig::make_default()
    {
        VehicleConfig config;

        VehicleAxleConfig front;
        front.position = vec3(0.0f, 0.34f, 1.43f);
        front.steered = true;
        front.driven = true;
        front.brake_torque = 2000.0f;
        front.anti_roll_ratio = 0.6f;
        front.tire_lateral_stiffness = 20.0f;

        VehicleAxleConfig rear;
        rear.position = vec3(0.0f, 0.34f, -1.43f);
        rear.driven = true;
        rear.brake_torque = 1200.0f;
        rear.handbrake_torque = 3000.0f;
        rear.anti_roll_ratio = 0.3f;
        rear.tire_lateral_stiffness = 28.0f;

        config.axles = {front, rear};

        return config;
    }
}
