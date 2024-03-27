// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once
#include <px/core/px_physics_engine.h>

#if PX_VEHICLE

namespace physics
{
#if PX_VEHICLE
	using namespace physx;
	using namespace physx::vehicle2;
	using namespace snippetvehicle2;
#endif

	static inline const char* vehicleDataPath = "E:\\Era Engine\\ext\\PhysX\\physx\\vehicledata";

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

	inline void initMaterialFrictionTable(px_physics_engine* physics)
	{
		material = physics->getPhysics()->
			createMaterial(0.8f, 0.8f, 0.6f);

		materialFrictions[0].friction = 1.0f;
		materialFrictions[0].material = material;
		defaultMaterialFriction = 1.0f;
		nbMaterialFrictions = 1;
	}

	inline bool initVehicle(px_physics_engine* physics)
	{
		BaseVehicleParams baseParams;
		if (!readBaseParamsFromJsonFile(vehicleDataPath, "Base.json", baseParams))
			return false;

		EngineDrivetrainParams engineDrivetrainParams;
		if (!readEngineDrivetrainParamsFromJsonFile(vehicleDataPath, "EngineDrive.json",
			engineDrivetrainParams))
			return false;

		initMaterialFrictionTable(physics);

		readBaseParamsFromJsonFile(vehicleDataPath, "Base.json", vehicle.mBaseParams);
		setPhysXIntegrationParams(vehicle.mBaseParams.axleDescription,
			materialFrictions, nbMaterialFrictions, defaultMaterialFriction,
			vehicle.mPhysXParams);
		readEngineDrivetrainParamsFromJsonFile(vehicleDataPath, "EngineDrive.json",
			vehicle.mEngineDriveParams);

		if (!vehicle.initialize(*physics->getPhysics(), PxCookingParams(PxTolerancesScale()), *material, EngineDriveVehicle::eDIFFTYPE_FOURWHEELDRIVE))
			return false;

		PxTransform pose(PxVec3(0.000000000f, -0.0500000119f, -1.59399998f), PxQuat(PxIdentity));
		vehicle.setUpActor(*physics->getScene(), pose, vehicleName);

		vehicle.mEngineDriveState.gearboxState.currentGear = vehicle.mEngineDriveParams.gearBoxParams.neutralGear + 1;
		vehicle.mEngineDriveState.gearboxState.targetGear = vehicle.mEngineDriveParams.gearBoxParams.neutralGear + 1;

		vehicle.mTransmissionCommandState.targetGear = PxVehicleEngineDriveTransmissionCommandState::eAUTOMATIC_GEAR;

		vehicleSimulationContext.setToDefault();
		vehicleSimulationContext.frame.lngAxis = PxVehicleAxes::ePosZ;
		vehicleSimulationContext.frame.latAxis = PxVehicleAxes::ePosX;
		vehicleSimulationContext.frame.vrtAxis = PxVehicleAxes::ePosY;
		vehicleSimulationContext.scale.scale = 1.0f;
		vehicleSimulationContext.gravity = physics::gravity;
		vehicleSimulationContext.physxScene = physics->getScene();
		vehicleSimulationContext.physxActorUpdateMode = PxVehiclePhysXActorUpdateMode::eAPPLY_ACCELERATION;

		return true;
	}

	inline void vehicleStep(float dt)
	{
		const px_command& command = commands[commandProgress];
		vehicle.mCommandState.brakes[0] = command.brake;
		vehicle.mCommandState.nbBrakes = 1;
		vehicle.mCommandState.throttle = command.throttle;
		vehicle.mCommandState.steer = command.steer;
		vehicle.mTransmissionCommandState.targetGear = command.gear;

		const PxVec3 linVel = vehicle.mPhysXState.physxActor.rigidBody->getLinearVelocity();
		const PxVec3 forwardDir = vehicle.mPhysXState.physxActor.rigidBody->getGlobalPose().q.getBasisVector2();
		const PxReal forwardSpeed = linVel.dot(forwardDir);
		const PxU8 nbSubsteps = (forwardSpeed < 5.0f ? 3 : 1);
		vehicle.mComponentSequence.setSubsteps(vehicle.mComponentSequenceSubstepGroupHandle, nbSubsteps);
		vehicle.step(dt, vehicleSimulationContext);
	}

	inline void vehiclePostStep(float dt)
	{
		commandTime += dt;
		if (commandTime > commands[commandProgress].duration)
		{
			commandProgress++;
			commandTime = 0.0f;
		}
	}

	inline void cleanupVehicle()
	{
		vehicle.destroy();
	}
}

#endif