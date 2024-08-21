// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include "px/core/px_physics_engine.h"
#include "core/project.h"
#include "core/string.h"

#if PX_VEHICLE

namespace era_engine::physics
{
#if PX_VEHICLE
	using namespace physx;
	using namespace physx::vehicle2;
	using namespace snippetvehicle2;
#endif

	static inline EngineDriveVehicle vehicle;

	static inline PxVehiclePhysXSimulationContext vehicleSimulationContext;

	static inline PxVehiclePhysXMaterialFriction materialFrictions[16];
	static inline PxU32 nbMaterialFrictions = 0;
	static inline PxReal defaultMaterialFriction = 1.0f;

	static inline const char vehicleName[] = "engineDrive";

	static inline const PxU32 targetGearCommand = PxVehicleEngineDriveTransmissionCommandState::eAUTOMATIC_GEAR;

	static inline PxMaterial* material = NULL;

	struct px_command
	{
		PxF32 brake;
		PxF32 throttle;
		PxF32 steer;
		PxU32 gear;
		PxF32 duration;
	};

	static inline px_command commands[] =
	{
		{0.5f, 0.0f, 0.0f, targetGearCommand, 2.0f},	//brake on and come to rest for 2 seconds
		{0.0f, 0.65f, 0.0f, targetGearCommand, 5.0f},	//throttle for 5 seconds
		{0.5f, 0.0f, 0.0f, targetGearCommand, 5.0f},	//brake for 5 seconds
		{0.0f, 0.75f, 0.0f, targetGearCommand, 5.0f},	//throttle for 5 seconds
		{0.0f, 0.25f, 0.5f, targetGearCommand, 5.0f}	//light throttle and steer for 5 seconds.
	};
	static inline const PxU32 nbCommands = sizeof(commands) / sizeof(px_command);
	static inline PxReal commandTime = 0.0f;
	static inline PxU32 commandProgress = 0;

	void initMaterialFrictionTable(px_physics_engine* physics);

	bool initVehicle(px_physics_engine* physics);

	void vehicleStep(float dt);

	void vehiclePostStep(float dt);

	void cleanupVehicle();
}

#endif