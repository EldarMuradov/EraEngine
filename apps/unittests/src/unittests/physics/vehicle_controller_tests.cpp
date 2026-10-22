#include <gtest/gtest.h>

#include <physics/vehicles/vehicle_controller.h>

namespace
{
    using namespace era_engine;
    using namespace era_engine::physics;

    constexpr float dt = 1.0f / 60.0f;

    VehicleCommands run(VehicleController& controller, const VehicleDriverInput& input, float forward_speed, uint32 steps)
    {
        const VehicleConfig config = VehicleConfig::make_default();

        VehicleCommands commands;
        for (uint32 i = 0; i < steps; ++i)
        {
            commands = controller.update(input, config.control, config.steering, forward_speed, dt);
        }
        return commands;
    }
}

TEST(VehicleControllerTest, ThrottleRampsUpWithConfiguredRate)
{
    VehicleController controller;
    VehicleDriverInput input;
    input.throttle = 1.0f;

    const float rise = VehicleConfig::make_default().control.throttle_rise;

    const VehicleCommands commands = run(controller, input, 0.0f, 6);

    EXPECT_NEAR(commands.throttle, rise * 6.0f * dt, 1e-4f);
    EXPECT_FALSE(commands.reverse);
    EXPECT_EQ(commands.clutch, 0.0f);
}

TEST(VehicleControllerTest, BrakeAtSpeedDoesNotReverse)
{
    VehicleController controller;
    VehicleDriverInput input;
    input.brake = 1.0f;

    const VehicleCommands commands = run(controller, input, 10.0f, 60);

    EXPECT_FALSE(commands.reverse);
    EXPECT_FLOAT_EQ(commands.brake, 1.0f);
    EXPECT_FLOAT_EQ(commands.throttle, 0.0f);
}

TEST(VehicleControllerTest, BrakeWhenStoppedEngagesReverse)
{
    VehicleController controller;
    VehicleDriverInput input;
    input.brake = 1.0f;

    VehicleCommands commands = run(controller, input, 0.0f, 60);

    EXPECT_TRUE(commands.reverse);
    EXPECT_FLOAT_EQ(commands.throttle, 1.0f);
    EXPECT_FLOAT_EQ(commands.brake, 0.0f);

    // Throttle while rolling backwards brakes first.
    input = {};
    input.throttle = 1.0f;
    commands = run(controller, input, -5.0f, 60);

    EXPECT_TRUE(commands.reverse);
    EXPECT_FLOAT_EQ(commands.brake, 1.0f);

    commands = run(controller, input, -0.2f, 1);
    EXPECT_FALSE(commands.reverse);
}

TEST(VehicleControllerTest, SteeringIsReducedAtHighSpeed)
{
    const VehicleConfig config = VehicleConfig::make_default();

    VehicleDriverInput input;
    input.steer = 1.0f;

    VehicleController slow;
    EXPECT_FLOAT_EQ(run(slow, input, 0.0f, 120).steer, 1.0f);

    VehicleController fast;
    EXPECT_NEAR(run(fast, input, config.steering.high_speed * 2.0f, 120).steer, config.steering.high_speed_angle / config.steering.max_angle, 1e-4f);
}

TEST(VehicleControllerTest, SteeringReturnsFasterThanItTurns)
{
    VehicleController controller;
    VehicleDriverInput input;
    input.steer = 1.0f;
    run(controller, input, 0.0f, 120);

    input.steer = 0.0f;
    const VehicleCommands commands = run(controller, input, 0.0f, 6);

    const VehicleControlConfig control = VehicleConfig::make_default().control;
    EXPECT_NEAR(commands.steer, 1.0f - control.steer_return_rate * 6.0f * dt, 1e-4f);
}

TEST(VehicleControllerTest, HandbrakeDisengagesClutch)
{
    VehicleController controller;
    VehicleDriverInput input;
    input.throttle = 1.0f;
    input.handbrake = true;

    const VehicleCommands commands = run(controller, input, 20.0f, 10);

    EXPECT_FLOAT_EQ(commands.handbrake, 1.0f);
    EXPECT_FLOAT_EQ(commands.clutch, 1.0f);
}

TEST(VehicleControllerTest, CoastBrakeWithoutPedals)
{
    VehicleController controller;
    const VehicleCommands commands = run(controller, VehicleDriverInput{}, 5.0f, 10);

    EXPECT_FLOAT_EQ(commands.brake, VehicleConfig::make_default().control.coast_brake);
    EXPECT_FLOAT_EQ(commands.throttle, 0.0f);
}
