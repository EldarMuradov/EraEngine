// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "application.h"
#include "px/core/px_physics_engine.h"
#include "px/core/px_aggregate.h"
#include "px/physics/px_joint.h"

#/*define RG_NB_ACTORS 14
#define RG_NB_JOINTS 13

struct px_ragdoll
{
	px_ragdoll() = default;
	px_ragdoll(escene* sc);
	px_ragdoll(eentity rt, escene* sc) : root(rt), scene(sc) { init(); };
	~px_ragdoll() {}

	void init() noexcept;
	void release() noexcept;

	union
	{
		struct
		{
			eentity torso;
			eentity head;
			eentity leftUpperArm;
			eentity leftLowerArm;
			eentity rightUpperArm;
			eentity rightLowerArm;
			eentity leftUpperLeg;
			eentity leftLowerLeg;
			eentity leftFoot;
			eentity leftToes;
			eentity rightUpperLeg;
			eentity rightLowerLeg;
			eentity rightFoot;
			eentity rightToes;
		};
		eentity bodyParts[RG_NB_ACTORS];
	};

	union
	{
		struct
		{
			px_joint* torsoHead;
			px_joint* leftArm;
			px_joint* leftArmTorso;
			px_joint* rightArm;
			px_joint* rightArmTorso;
			px_joint* leftLeg;
			px_joint* leftLegTorso;
			px_joint* rightLeg;
			px_joint* rightLegTorso;
			px_joint* leftFootLeg;
			px_joint* rigthFootLeg;
			px_joint* leftToesArm;
			px_joint* rightToesArm;
		};
		px_joint* joints = new px_joint[RG_NB_JOINTS];
	};

private:
	void createActors();
	void createBaseMesh();

private:
	eentity root;

	escene* scene = nullptr;

	px_aggregate* group = nullptr;

	std::unordered_map<const char*, px_rigidbody_component*> rigidbodies;

	ref<multi_mesh> meshBase;
};*/