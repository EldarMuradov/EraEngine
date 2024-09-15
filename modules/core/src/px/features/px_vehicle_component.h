// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "px/features/px_vehicles.h"

namespace era_engine
{
	struct eentity;
}

#if PX_VEHICLE

namespace era_engine::physics
{
	struct px_vehicle_base_component : px_physics_component_base
	{
		struct px_default_vehicle_command
		{
			PxF32 brake;
			PxF32 throttle;
			PxF32 steer;
			PxU32 gear;
			PxF32 duration;
		};

		px_vehicle_base_component() = default;
		px_vehicle_base_component(uint32 handle, const vec3& position);

		virtual ~px_vehicle_base_component();

		virtual void setupVehicle();
		virtual void simulationStep(float dt);
		virtual void simulationPostStep(float dt);

		virtual void release(bool release = true) override;

		trs getTransform() const;
		void setTransform(const vec3& pos, const quat& rot);

	protected:
		virtual void initMaterialFrictionTable();

	protected:
		EngineDriveVehicle* vehicle = nullptr;
		PxVehiclePhysXSimulationContext* vehicleSimulationContext = nullptr;
		PxMaterial* material = nullptr;

		PxVehiclePhysXMaterialFriction materialFrictions[16];

		PxU32 nbMaterialFrictions = 0;
		PxReal defaultMaterialFriction = 1.0f;

		const char* vehicleName = "engineDrive";

		PxU32 targetGearCommand = PxVehicleEngineDriveTransmissionCommandState::eAUTOMATIC_GEAR;

		PxReal commandTime = 0.0f;
		PxU32 commandProgress = 0;

	protected:
		std::vector<px_default_vehicle_command> commands;
	};

	struct px_4_wheels_vehicle_component : px_vehicle_base_component
	{
		px_4_wheels_vehicle_component() = default;
		px_4_wheels_vehicle_component(uint32 handle, const vec3& position);

		virtual ~px_4_wheels_vehicle_component();
	};

	struct px_tank_vehicle_component : px_vehicle_base_component
	{
		struct px_tank_command
		{
			PxF32 brake0;			//Tanks have two brake controllers:
			PxF32 brake1;			//  one brake controller for the left track and one for the right track.
			PxF32 thrust0;			//Tanks have two thrust controllers that divert engine torque to the left and right tracks:
			PxF32 thrust1;			//  one thrust controller for the left track and one for the right track.
			PxF32 throttle;			//Tanks are driven by an engine that requires a throttle to generate engine drive torque.
			PxU32 gear;				//Tanks are geared and may use automatic gearing.
			PxF32 duration;
		};

		px_tank_vehicle_component() = default;
		px_tank_vehicle_component(uint32 handle, const vec3& position);

		void setupVehicle() override;

		virtual ~px_tank_vehicle_component();

	private:
		std::vector<px_tank_command> commands;
	};

	px_vehicle_base_component* getVehicleComponent(eentity& entity);
}

#endif