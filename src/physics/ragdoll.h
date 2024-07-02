// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "physics.h"
#include "scene/scene.h"

namespace era_engine
{
	struct humanoid_ragdoll
	{
		humanoid_ragdoll() {}

		void initialize(escene& scene, vec3 initialHipPosition, float initialRotation = 0.f);
		static humanoid_ragdoll create(escene& scene, vec3 initialHipPosition, float initialRotation = 0.f);

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
			eentity bodyParts[14];
		};

		union
		{
			struct
			{
				eentity torsoParent;
				eentity headParent;
				eentity leftUpperArmParent;
				eentity leftLowerArmParent;
				eentity rightUpperArmParent;
				eentity rightLowerArmParent;
				eentity leftUpperLegParent;
				eentity leftLowerLegParent;
				eentity leftFootParent;
				eentity leftToesParent;
				eentity rightUpperLegParent;
				eentity rightLowerLegParent;
				eentity rightFootParent;
				eentity rightToesParent;
			};
			eentity bodyPartParents[14];
		};

		union
		{
			struct
			{
				cone_twist_constraint_handle neckConstraint;
				cone_twist_constraint_handle leftShoulderConstraint;
				cone_twist_constraint_handle rightShoulderConstraint;
				cone_twist_constraint_handle leftHipConstraint;
				cone_twist_constraint_handle leftAnkleConstraint;
				cone_twist_constraint_handle rightHipConstraint;
				cone_twist_constraint_handle rightAnkleConstraint;

				hinge_constraint_handle leftElbowConstraint;
				hinge_constraint_handle rightElbowConstraint;
				hinge_constraint_handle leftKneeConstraint;
				hinge_constraint_handle leftToesConstraint;
				hinge_constraint_handle rightKneeConstraint;
				hinge_constraint_handle rightToesConstraint;
			};
			struct
			{
				cone_twist_constraint_handle coneTwistConstraints[7];
				hinge_constraint_handle hingeConstraints[6];
			};
		};
	};

}