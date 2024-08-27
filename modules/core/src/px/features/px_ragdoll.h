// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "px/core/px_physics_engine.h"
#include "px/core/px_aggregate.h"
#include "px/physics/px_joint.h"

namespace era_engine::animation
{
	struct animation_skeleton;
}

namespace era_engine::physics
{
	struct px_ragdoll
	{
		std::vector<PxRigidDynamic*> rigidBodies;
		std::vector<vec3> relativeJointPoses;
		std::vector<quat> originalBodyRotations;

		std::vector<vec3> bodyPosRelativeToJoint;
		std::vector<quat> originalJointRotations;

		PxRigidDynamic* findRecentBody(uint32_t idx, ref<animation::animation_skeleton> skeleton, uint32_t& chosenIdx);
		void setKinematic(bool state);
	};

	struct px_ragdoll_component : px_physics_component_base
	{
		px_ragdoll_component() = default;
		px_ragdoll_component(uint32 handle, const vec3& pos = vec3(0.0f));

		~px_ragdoll_component();

		void initRagdoll(ref<animation::animation_skeleton> skeletonRef);

		std::vector<trs> apply(const mat4& modelRotation = mat4::identity);

		void update(float dt);

		virtual void release(bool releaseActor = false) override;

		ref<px_ragdoll> ragdoll = nullptr;
		ref<animation::animation_skeleton> skeleton = nullptr;
		std::vector<trs> transforms;
		float scale = 0.1f;
		vec3 startPos;
		bool simulated = false;

		mat4 model = mat4::identity * scale;
		mat4 modelWithoutScale = mat4::identity;
		mat4 modelOnlyScale = mat4::identity * scale;
	};
}