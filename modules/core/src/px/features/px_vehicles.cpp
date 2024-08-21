// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/features/px_vehicles.h"



namespace era_engine::physics
{
#if PX_VEHICLE
	using namespace physx::vehicle2;
	using namespace snippetvehicle2;

	bool initVehicle(px_physics_engine* physics)
	{
		PxRigidBody* actor = nullptr;
		const char* path = wstringToString((eproject::enginePath + L"/resources/vehicledata")).c_str();
		BaseVehicleParams baseParams;
		if (!readBaseParamsFromJsonFile(path, "Base.json", baseParams))
			return false;

		EngineDrivetrainParams engineDrivetrainParams;
		if (!readEngineDrivetrainParamsFromJsonFile(path, "EngineDrive.json",
			engineDrivetrainParams))
			return false;

		initMaterialFrictionTable(physics);

		readBaseParamsFromJsonFile(path, "Base.json", vehicle.mBaseParams);
		setPhysXIntegrationParams(vehicle.mBaseParams.axleDescription,
			materialFrictions, nbMaterialFrictions, defaultMaterialFriction,
			vehicle.mPhysXParams);
		readEngineDrivetrainParamsFromJsonFile(path, "EngineDrive.json",
			vehicle.mEngineDriveParams);
		actor = vehicle.mPhysXState.physxActor.rigidBody;
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

	void initMaterialFrictionTable(px_physics_engine* physics)
	{
		material = physics->getPhysics()->
			createMaterial(0.8f, 0.8f, 0.6f);

		materialFrictions[0].friction = 1.0f;
		materialFrictions[0].material = material;
		defaultMaterialFriction = 1.0f;
		nbMaterialFrictions = 1;
	}

	void vehicleStep(float dt)
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

	void vehiclePostStep(float dt)
	{
		commandTime += dt;
		if (commandTime > commands[commandProgress].duration)
		{
			commandProgress++;
			commandTime = 0.0f;
		}
	}

	void cleanupVehicle()
	{
		vehicle.destroy();
	}

#endif

}

