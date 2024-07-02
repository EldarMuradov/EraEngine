// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "physics.h"

namespace era_engine
{
	struct vehicle
	{
		vehicle() {}

		void initialize(escene& scene, vec3 initialMotorPosition, float initialRotation = 0.f);
		static vehicle create(escene& scene, vec3 initialMotorPosition, float initialRotation = 0.f);

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

}