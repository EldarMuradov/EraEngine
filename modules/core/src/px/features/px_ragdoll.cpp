// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/features/px_ragdoll.h"
#include "px/physics/px_collider_component.h"

#include "animation/animation.h"

#include "scene/components.h"

namespace era_engine::physics
{
	static void finishBody(PxRigidDynamic* dyn, PxReal density, PxReal inertiaScale)
	{
		//dyn->setSolverIterationCounts(DEFAULT_SOLVER_ITERATIONS);
		dyn->setMaxDepenetrationVelocity(2.f);
		PxRigidBodyExt::updateMassAndInertia(*dyn, density);
		dyn->setMassSpaceInertiaTensor(dyn->getMassSpaceInertiaTensor() * inertiaScale);
		dyn->setAngularDamping(0.15f);
	}

	static void configD6Joint(PxReal swing0, PxReal swing1, PxReal twistLo, PxReal twistHi, PxD6Joint* joint)
	{
		joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
		joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
		joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);

		joint->setSwingLimit(PxJointLimitCone(swing0, swing1));
		joint->setTwistLimit(PxJointAngularLimitPair(twistLo, twistHi));
	}

    static vec3 bindPosWs(mat4& inverseBindPose, mat4& model)
    {
        mat4 m = model * invert(inverseBindPose);
        return vec3(m.m30, m.m31, m.m32);
    }

    static PxRigidDynamic* createCapsuleBone(
        uint32_t parentIdx,
        uint32_t childIdx,
        px_ragdoll& ragdoll,
        PxMaterial* material,
        ref<animation::animation_skeleton> skeleton,
        mat4 modelWithoutScale = mat4::identity,
        mat4 modelWithScale = mat4::identity,
        mat4 model = mat4::identity,
        float r = 0.5f,
        mat4 rotation = mat4::identity,
        float mass = 1.0f)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;

        vec3 parentPos = bindPosWs(joints[parentIdx].invBindTransform, model);
        vec3 childPos = bindPosWs(joints[childIdx].invBindTransform, model);

        float len = length(parentPos - childPos);

        // shorten by 0.1 times to prevent overlap
        len -= len * 0.1f;

        float lenMinus2r = len - 2.0f * r;
        float halfHeight = lenMinus2r / 2.0f;

        vec3 bodyPos = (parentPos + childPos) / 2.0f;

        PxShape* shape = physics->getPhysics()->createShape(PxCapsuleGeometry(r, lenMinus2r), *material);

        PxTransform local(createPxQuat(mat4ToTRS(rotation).rotation));
        shape->setLocalPose(local);

        mat4 invBindPose = joints[parentIdx].invBindTransform;
        mat4 bindPose = invert(invBindPose);
        mat4 bindPoseWs = modelWithoutScale * bindPose;

        PxTransform px_transform(createPxVec3(bodyPos), createPxQuat(mat4ToTRS(bindPoseWs).rotation));

        PxRigidDynamic* body = physics->getPhysics()->createRigidDynamic(px_transform);
        body->setMass(mass);

        body->attachShape(*shape);

        ragdoll.rigidBodies[parentIdx] = body;

        return body;
    }

    static PxRigidDynamic* createCapsuleBone(
        uint32_t parentIdx,
        px_ragdoll& ragdoll,
        PxMaterial* material,
        vec3 offset,
        float l,
        ref<animation::animation_skeleton> skeleton,
        mat4 modelWithoutScale = mat4::identity,
        mat4 modelWithScale = mat4::identity,
        mat4 model = mat4::identity,
        float r = 0.5f,
        mat4 rotation = mat4::identity,
        float mass = 1.0f)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;

        vec3 parentPos = bindPosWs(joints[parentIdx].invBindTransform, model) + offset;

        PxShape* shape = physics->getPhysics()->createShape(PxCapsuleGeometry(r, l), *material);

        PxTransform local(createPxQuat(mat4ToTRS(rotation).rotation));
        shape->setLocalPose(local);

        mat4 invBindPose = joints[parentIdx].invBindTransform;
        mat4 bindPose = invert(invBindPose);
        mat4 bindPoseWs = modelWithoutScale * bindPose;

        PxTransform px_transform(createPxVec3(parentPos), createPxQuat(mat4ToTRS(bindPoseWs).rotation));

        PxRigidDynamic* body = physics->getPhysics()->createRigidDynamic(px_transform);

        body->attachShape(*shape);
        body->setMass(mass);

        ragdoll.rigidBodies[parentIdx] = body;

        return body;
    }

    static PxRigidDynamic* createSphereBone(
        uint32_t parentIdx,
        px_ragdoll& ragdoll,
        PxMaterial* material,
        float r,
        ref<animation::animation_skeleton> skeleton,
        mat4 model = mat4::identity,
        float mass = 1.0f)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;

        vec3 parentPos = bindPosWs(joints[parentIdx].invBindTransform, model);

        PxShape* shape = physics->getPhysics()->createShape(PxSphereGeometry(r), *material);

        PxRigidDynamic* body = physics->getPhysics()->createRigidDynamic(PxTransform(createPxVec3(parentPos)));

        body->attachShape(*shape);
        body->setMass(mass);

        ragdoll.rigidBodies[parentIdx] = body;

        return body;
    }

    static void createD6Joint(
        PxRigidDynamic* parent,
        PxRigidDynamic* child,
        ref<animation::animation_skeleton> skeleton,
        uint32_t jointPos,
        mat4 model = mat4::identity)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;
        vec3 p = bindPosWs(joints[jointPos].invBindTransform, model);
        quat q = mat4ToTRS(invert(joints[jointPos].invBindTransform)).rotation;

        PxD6Joint* joint = PxD6JointCreate(*physics->getPhysics(),
            parent,
            parent->getGlobalPose().transformInv(PxTransform(createPxVec3(p), createPxQuat(q))),
            child,
            child->getGlobalPose().transformInv(PxTransform(createPxVec3(p), createPxQuat(q))));

        joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        configD6Joint(3.14 / 4.f, 3.14f / 4.f, -3.14f / 8.f, 3.14f / 8.f, joint);
    }

    static void createRevoluteJoint(
        PxRigidDynamic* parent,
        PxRigidDynamic* child,
        ref<animation::animation_skeleton> skeleton,
        uint32_t jointPos,
        mat4 rotation = mat4::identity,
        mat4 model = mat4::identity)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;
        vec3 p = bindPosWs(joints[jointPos].invBindTransform, model);

        mat4 parentPoseInv = invert(createMat44(parent->getGlobalPose()));
        mat4 childPoseInv = invert(createMat44(child->getGlobalPose()));
        mat4 jointTransform = mat4::identity;
        jointTransform = trsToMat4(trs{ p }) * rotation;

        PxRevoluteJoint* joint = PxRevoluteJointCreate(*physics->getPhysics(),
            parent,
            PxTransform(createPxMat44(parentPoseInv * jointTransform)),
            child,
            PxTransform(createPxMat44(childPoseInv * jointTransform)));

        joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        joint->setLimit(PxJointAngularLimitPair(0.0f, PxPi));
        joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
    }


	px_ragdoll_component::px_ragdoll_component(uint32 handle, const vec3& pos)
		: px_physics_component_base(handle)
	{
	}

	px_ragdoll_component::~px_ragdoll_component()
	{
	}

	std::vector<trs> px_ragdoll_component::apply(const mat4& modelScale, const mat4& modelRotation)
	{
		using namespace animation;

		if (ragdoll->rigidBodies.size() > 0)
		{
			std::vector<skeleton_joint>& joints = skeleton->joints;

			for (uint32_t i = 0; i < (size_t)limb_type_count; i++)
			{
				uint32_t chosenIdx;
				PxRigidDynamic* body = ragdoll->findRecentBody(i, skeleton, chosenIdx);

				mat4 globalTransform = createMat44(body->getGlobalPose());
				vec4 globalJointPos = globalTransform * vec4(ragdoll->relativeJointPoses[i], 1.0f);

				quat bodyRot = mat4ToTRS(globalTransform).rotation;
				quat diffRot = conjugate(ragdoll->originalBodyRotations[i]) * bodyRot;

				mat4 translation = trsToMat4(trs(vec3(globalJointPos.x, globalJointPos.y, globalJointPos.z)));

				quat finalRotation = mat4ToTRS(joints[i].bindTransform).rotation * diffRot;
				mat4 rotation = modelRotation * quaternionToMat4(finalRotation);

				mat4 model = modelRotation * modelScale;

				transforms[i] = mat4ToTRS(invert(model) * translation * rotation * modelScale);
			}
		}

		return transforms;
	}

	void px_ragdoll_component::release(bool releaseActor)
	{
		ragdoll.reset();
	}

	PxRigidDynamic* px_ragdoll::findRecentBody(uint32_t idx, ref<animation::animation_skeleton> skeleton, uint32_t& chosenIdx)
	{
		using namespace animation;

		std::vector<skeleton_joint>& joints = skeleton->joints;

		chosenIdx = idx;
		PxRigidDynamic* body = rigidBodies[idx];

		while (!body)
		{
			idx = joints[idx].parentID;
			body = rigidBodies[idx];
			chosenIdx = idx;
		}

		return body;
	}

	void px_ragdoll::setKinematic(bool state)
	{
		for (size_t i = 0; i < rigidBodies.size(); ++i)
		{
			if (rigidBodies[i])
			{
				rigidBodies[i]->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, state);

				if (!state)
					rigidBodies[i]->wakeUp();
			}
		}
	}

}