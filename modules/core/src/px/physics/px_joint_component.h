// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "px/core/px_physics_engine.h"

#include "px/physics/px_joint.h"

namespace era_engine::physics
{
	using namespace physx;

	struct px_fixed_joint;

	struct px_fixed_joint_component : px_physics_component_base
	{
		px_fixed_joint_component() {};
		px_fixed_joint_component(uint32 handle, float breakForce = 200.0f);
		virtual ~px_fixed_joint_component();

		void setPair(PxRigidActor* f, PxRigidActor* s);

		px_fixed_joint* getJoint() const { return joint; }

		virtual void release(bool releaseActor = false) override;

	private:
		px_fixed_joint* joint = nullptr;
		float jointBreakForce = 200.0f;
	};
}