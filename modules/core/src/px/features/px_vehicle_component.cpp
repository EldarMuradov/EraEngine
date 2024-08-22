// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/features/px_vehicle_component.h"

#include "scene/scene.h"

namespace era_engine::physics
{
#if PX_VEHICLE

	using namespace physx::vehicle2;
	using namespace snippetvehicle2;

	px_vehicle_base_component::px_vehicle_base_component(uint32 handle, const vec3& position)
		: px_physics_component_base(handle)
	{
		vehicle = new EngineDriveVehicle();
		vehicleSimulationContext = new PxVehiclePhysXSimulationContext();
	}

	px_vehicle_base_component::~px_vehicle_base_component()
	{
	}

	void px_vehicle_base_component::setupVehicle()
	{
		const auto& physics = physics_holder::physicsRef;

		const char* path = getAssetPath("/resources/assets/vehicledata").c_str();
		BaseVehicleParams baseParams;
		if (!readBaseParamsFromJsonFile(path, "Base.json", baseParams))
		{
			LOG_ERROR("");
			return;
		}

		EngineDrivetrainParams engineDrivetrainParams;
		if (!readEngineDrivetrainParamsFromJsonFile(path, "EngineDrive.json",
			engineDrivetrainParams))
		{
			LOG_ERROR("");
			return;
		}

		initMaterialFrictionTable();

		readBaseParamsFromJsonFile(path, "Base.json", vehicle->mBaseParams);
		setPhysXIntegrationParams(vehicle->mBaseParams.axleDescription,
			materialFrictions, nbMaterialFrictions, defaultMaterialFriction,
			vehicle->mPhysXParams);
		readEngineDrivetrainParamsFromJsonFile(path, "EngineDrive.json",
			vehicle->mEngineDriveParams);

		if (!vehicle->initialize(*physics->getPhysics(), PxCookingParams(PxTolerancesScale()), *material, EngineDriveVehicle::eDIFFTYPE_FOURWHEELDRIVE))
		{
			LOG_ERROR("");
			return;
		}

		PxTransform pose(PxVec3(0.000000000f, -0.0500000119f, -1.59399998f), PxQuat(PxIdentity));
		vehicle->setUpActor(*physics->getScene(), pose, vehicleName);

		vehicle->mEngineDriveState.gearboxState.currentGear = vehicle->mEngineDriveParams.gearBoxParams.neutralGear + 1;
		vehicle->mEngineDriveState.gearboxState.targetGear = vehicle->mEngineDriveParams.gearBoxParams.neutralGear + 1;

		vehicle->mTransmissionCommandState.targetGear = PxVehicleEngineDriveTransmissionCommandState::eAUTOMATIC_GEAR;

		vehicleSimulationContext->setToDefault();
		vehicleSimulationContext->frame.lngAxis = PxVehicleAxes::ePosZ;
		vehicleSimulationContext->frame.latAxis = PxVehicleAxes::ePosX;
		vehicleSimulationContext->frame.vrtAxis = PxVehicleAxes::ePosY;
		vehicleSimulationContext->scale.scale = 1.0f;
		vehicleSimulationContext->gravity = physics::gravity;
		vehicleSimulationContext->physxScene = physics->getScene();
		vehicleSimulationContext->physxActorUpdateMode = PxVehiclePhysXActorUpdateMode::eAPPLY_ACCELERATION;

		commands = {
			{ 0.5f, 0.0f, 0.0f, targetGearCommand, 2.0f },	//brake on and come to rest for 2 seconds
			{ 0.0f, 0.65f, 0.0f, targetGearCommand, 5.0f },	//throttle for 5 seconds
			{ 0.5f, 0.0f, 0.0f, targetGearCommand, 5.0f },	//brake for 5 seconds
			{ 0.0f, 0.75f, 0.0f, targetGearCommand, 5.0f },	//throttle for 5 seconds
			{ 0.0f, 0.25f, 0.5f, targetGearCommand, 5.0f }	//light throttle and steer for 5 seconds.
		};
	}

	void px_vehicle_base_component::simulationStep(float dt)
	{
		const px_default_vehicle_command& command = commands[commandProgress];
		vehicle->mCommandState.brakes[0] = command.brake;
		vehicle->mCommandState.nbBrakes = 1;
		vehicle->mCommandState.throttle = command.throttle;
		vehicle->mCommandState.steer = command.steer;
		vehicle->mTransmissionCommandState.targetGear = command.gear;

		const PxVec3 linVel = vehicle->mPhysXState.physxActor.rigidBody->getLinearVelocity();
		const PxVec3 forwardDir = vehicle->mPhysXState.physxActor.rigidBody->getGlobalPose().q.getBasisVector2();
		const PxReal forwardSpeed = linVel.dot(forwardDir);
		const PxU8 nbSubsteps = (forwardSpeed < 5.0f ? 3 : 1);
		vehicle->mComponentSequence.setSubsteps(vehicle->mComponentSequenceSubstepGroupHandle, nbSubsteps);
		vehicle->step(dt, *vehicleSimulationContext);
	}

	void px_vehicle_base_component::simulationPostStep(float dt)
	{
		commandTime += dt;
		if (commandTime > commands[commandProgress].duration)
		{
			commandProgress++;
			commandTime = 0.0f;
		}
	}

	void px_vehicle_base_component::initMaterialFrictionTable()
	{
		const auto& physics = physics_holder::physicsRef;

		material = physics->getPhysics()->
			createMaterial(0.8f, 0.8f, 0.6f);

		materialFrictions[0].friction = 1.0f;
		materialFrictions[0].material = material;
		defaultMaterialFriction = 1.0f;
		nbMaterialFrictions = 1;
	}

	void px_vehicle_base_component::release(bool release)
	{
		vehicle->destroy();
		RELEASE_PTR(vehicle)
		RELEASE_PTR(vehicleSimulationContext)
	}

	trs px_vehicle_base_component::getTransform() const
	{
		PxTransform pose = vehicle->mPhysXState.physxActor.rigidBody->getGlobalPose();

		trs currentTrs;
		currentTrs.position = createVec3(pose.p);
		currentTrs.rotation = createQuat(pose.q);
		return currentTrs;
	}

	void px_vehicle_base_component::setTransform(const vec3& pos, const quat& rot)
	{
		physics_lock_write lock{};

		vehicle->mPhysXState.physxActor.rigidBody->setGlobalPose(PxTransform(createPxVec3(pos), createPxQuat(rot)));
	}

	px_4_wheels_vehicle_component::px_4_wheels_vehicle_component(uint32 handle, const vec3& position)
		: px_vehicle_base_component(handle, position)
	{
	}

	px_4_wheels_vehicle_component::~px_4_wheels_vehicle_component()
	{
	}

	px_tank_vehicle_component::px_tank_vehicle_component(uint32 handle, const vec3& position)
		: px_vehicle_base_component(handle, position)
	{
	}

	void px_tank_vehicle_component::setupVehicle()
	{
	}

	px_tank_vehicle_component::~px_tank_vehicle_component()
	{
	}

	px_vehicle_base_component* getVehicleComponent(eentity& entity)
	{
		if (auto* vehicle = entity.getComponentIfExists<px_vehicle_base_component>())
		{
			return vehicle;
		}
		else if (auto* vehicle = entity.getComponentIfExists<px_tank_vehicle_component>())
		{
			return vehicle;
		}
		else if (auto* vehicle = entity.getComponentIfExists<px_4_wheels_vehicle_component>())
		{
			return vehicle;
		}
		return nullptr;
	}

#endif
}