#pragma once

#include "physics.h"

struct vehicle
{
	vehicle() {}

	void initialize(game_scene& scene, vec3 initialMotorPosition, float initialRotation = 0.f);
	static vehicle create(game_scene& scene, vec3 initialMotorPosition, float initialRotation = 0.f);

	union
	{
		struct
		{
			eentity motor;
			eentity motorGear;
			eentity driveAxis;
			eentity frontAxis;
			eentity steeringWheel;
			eentity steeringAxis;

			eentity leftWheelSuspension;
			eentity rightWheelSuspension;

			eentity leftFrontWheel;
			eentity rightFrontWheel;

			eentity leftWheelArm;
			eentity rightWheelArm;

			eentity differentialSunGear;
			eentity differentialSpiderGear;

			eentity leftRearWheel;
			eentity rightRearWheel;
		};
		eentity parts[16];
	};
};
