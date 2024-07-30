// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/physics/px_joint_component.h"

namespace era_engine::physics
{
	px_fixed_joint_component::px_fixed_joint_component(uint32 handle, float breakForce)
		: jointBreakForce(breakForce), px_physics_component_base(handle)
	{
	}

	px_fixed_joint_component::~px_fixed_joint_component()
	{
	}

	void px_fixed_joint_component::setPair(PxRigidActor* f, PxRigidActor* s)
	{
		joint = new px_fixed_joint(px_fixed_joint_desc{10.0f, 10.0f, jointBreakForce, jointBreakForce}, f, s);
	}

	void px_fixed_joint_component::release(bool releaseActor)
	{
		RELEASE_PTR(joint)
	}

}