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
        return vec3(m.m03, m.m13, m.m23);
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

        vec3 parentPos = bindPosWs(joints[parentIdx].invBindTransform, model) * modelWithScale.m00;
        vec3 childPos = bindPosWs(joints[childIdx].invBindTransform, model) * modelWithScale.m00;

        float len = length(parentPos - childPos);

        // shorten by 0.1 times to prevent overlap
        len -= len * 0.1f;

        float lenMinus2r = len - 2.0f * r;
        float halfHeight = lenMinus2r / 2.0f;

        vec3 bodyPos = (parentPos + childPos) / 2.0f;

        PxShape* shape = physics->getPhysics()->createShape(PxCapsuleGeometry(r, halfHeight), *material);

        PxTransform local(createPxQuat(mat4ToTRS(rotation).rotation));
        shape->setLocalPose(local);
        shape->setSimulationFilterData(PxFilterData(-1, -1, 0, 0));
        shape->setQueryFilterData(PxFilterData(-1, -1, 0, 0));
        shape->setFlag(PxShapeFlag::eVISUALIZATION, true);
        shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);

        mat4 invBindPose = joints[parentIdx].invBindTransform;
        mat4 bindPose = invert(invBindPose);
        mat4 bindPoseWs = modelWithoutScale * bindPose;

        PxTransform px_transform(createPxVec3(bodyPos), createPxQuat(mat4ToTRS(bindPoseWs).rotation));

        PxRigidDynamic* body = physics->getPhysics()->createRigidDynamic(px_transform);
        body->setActorFlag(PxActorFlag::eVISUALIZATION, true);
        body->setMass(mass);
        //body->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, true);


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

        vec3 parentPos = (bindPosWs(joints[parentIdx].invBindTransform, model) + offset) * modelWithScale.m00;

        PxShape* shape = physics->getPhysics()->createShape(PxCapsuleGeometry(r, l / 2.0f), *material);

        PxTransform local(createPxQuat(mat4ToTRS(rotation).rotation));
        shape->setLocalPose(local);
        shape->setFlag(PxShapeFlag::eVISUALIZATION, true);
        shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
        shape->setSimulationFilterData(PxFilterData(-1, -1, 0, 0));
        shape->setQueryFilterData(PxFilterData(-1, -1, 0, 0));
        mat4 invBindPose = joints[parentIdx].invBindTransform;
        mat4 bindPose = invert(invBindPose);
        mat4 bindPoseWs = modelWithoutScale * bindPose;

        PxTransform px_transform(createPxVec3(parentPos), createPxQuat(mat4ToTRS(bindPoseWs).rotation));

        PxRigidDynamic* body = physics->getPhysics()->createRigidDynamic(px_transform);
        body->setActorFlag(PxActorFlag::eVISUALIZATION, true);
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
        float mass = 1.0f,
        float scale = 0.1f)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;

        vec3 parentPos = bindPosWs(joints[parentIdx].invBindTransform, model) * scale;

        PxShape* shape = physics->getPhysics()->createShape(PxSphereGeometry(r), *material);
        shape->setFlag(PxShapeFlag::eVISUALIZATION, true);
        shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
        shape->setSimulationFilterData(PxFilterData(-1, -1, 0, 0));
        shape->setQueryFilterData(PxFilterData(-1, -1, 0, 0));
        PxRigidDynamic* body = physics->getPhysics()->createRigidDynamic(PxTransform(createPxVec3(parentPos)));
        body->setActorFlag(PxActorFlag::eVISUALIZATION, true);
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
        mat4 model = mat4::identity,
        float scale = 0.1f)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;
        vec3 p = bindPosWs(joints[jointPos].invBindTransform, model) * scale;
        quat q = mat4ToTRS(invert(joints[jointPos].invBindTransform)).rotation;

        PxD6Joint* joint = PxD6JointCreate(*physics->getPhysics(),
            parent,
            parent->getGlobalPose().transformInv(PxTransform(createPxVec3(p), createPxQuat(q))),
            child,
            child->getGlobalPose().transformInv(PxTransform(createPxVec3(p), createPxQuat(q))));

        joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        configD6Joint(3.14f / 4.f, 3.14f / 4.f, -3.14f / 8.f, 3.14f / 8.f, joint);
    }

    static void createRevoluteJoint(
        PxRigidDynamic* parent,
        PxRigidDynamic* child,
        ref<animation::animation_skeleton> skeleton,
        uint32_t jointPos,
        mat4 rotation = mat4::identity,
        mat4 model = mat4::identity,
        float scale = 0.1f)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;
        vec3 p = bindPosWs(joints[jointPos].invBindTransform, model) * scale;

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
        joint->setLimit(PxJointAngularLimitPair(-PxPi / 2.0f, PxPi / 2.0f));
        joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
    }


	px_ragdoll_component::px_ragdoll_component(uint32 handle, const vec3& pos)
		: px_physics_component_base(handle), startPos(pos)
	{
	}

	px_ragdoll_component::~px_ragdoll_component()
	{
	}

    void px_ragdoll_component::initRagdoll(ref<animation::animation_skeleton> skeletonRef)
    {
        using namespace animation;

        skeleton = skeletonRef;

        ragdoll = make_ref<px_ragdoll>();

        const auto& physics = physics_holder::physicsRef;
        const auto& material = physics->getDefaultMaterial();
        PxScene* scene = physics->getScene();

        auto& joints = skeleton->joints;

        uint32_t j_head_idx = skeleton->nameToJointID["head"];
        uint32_t j_neck_01_idx = skeleton->nameToJointID["neck_01"];
        uint32_t j_spine_03_idx = skeleton->nameToJointID["spine_03"];
        uint32_t j_spine_02_idx = skeleton->nameToJointID["spine_02"];
        uint32_t j_spine_01_idx = skeleton->nameToJointID["spine_01"];
        uint32_t j_pelvis_idx = skeleton->nameToJointID["pelvis"];

        uint32_t j_thigh_l_idx = skeleton->nameToJointID["thigh_l"];
        uint32_t j_calf_l_idx = skeleton->nameToJointID["calf_l"];
        uint32_t j_foot_l_idx = skeleton->nameToJointID["foot_l"];
        uint32_t j_ball_l_idx = skeleton->nameToJointID["ball_l"];

        uint32_t j_thigh_r_idx = skeleton->nameToJointID["thigh_r"];
        uint32_t j_calf_r_idx = skeleton->nameToJointID["calf_r"];
        uint32_t j_foot_r_idx = skeleton->nameToJointID["foot_r"];
        uint32_t j_ball_r_idx = skeleton->nameToJointID["ball_r"];

        uint32_t j_upperarm_l_idx = skeleton->nameToJointID["upperarm_l"];
        uint32_t j_lowerarm_l_idx = skeleton->nameToJointID["lowerarm_l"];
        uint32_t j_hand_l_idx = skeleton->nameToJointID["hand_l"];
        uint32_t j_middle_01_l_idx = skeleton->nameToJointID["middle_01_l"];

        uint32_t j_upperarm_r_idx = skeleton->nameToJointID["upperarm_r"];
        uint32_t j_lowerarm_r_idx = skeleton->nameToJointID["lowerarm_r"];
        uint32_t j_hand_r_idx = skeleton->nameToJointID["hand_r"];
        uint32_t j_middle_01_r_idx = skeleton->nameToJointID["middle_01_r"];

       ragdoll->rigidBodies.resize(100);
       ragdoll->relativeJointPoses.resize(100);
       ragdoll->originalBodyRotations.resize(100);
       ragdoll->bodyPosRelativeToJoint.resize(100);
       ragdoll->originalJointRotations.resize(100);

        for (int i = 0; i < 100; i++)
            ragdoll->rigidBodies[i] = nullptr;

        float r = 5.0f * scale;
        mat4 rot = mat4::identity;

        PxRigidDynamic* pelvis = createCapsuleBone(
            j_pelvis_idx,
            j_neck_01_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            15.0f * scale,
            rot);

        PxRigidDynamic* head = createCapsuleBone(
            j_head_idx,
            *ragdoll,
            material,
            vec3(0.0f, 15.0f * scale, 0.0f),
            4.0f * scale,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            6.0f * scale,
            rot);

        PxRigidDynamic* l_leg = createCapsuleBone(
            j_thigh_l_idx,
            j_calf_l_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r,
            rot);

        PxRigidDynamic* r_leg = createCapsuleBone(
            j_thigh_r_idx,
            j_calf_r_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r,
            rot);

        PxRigidDynamic* l_calf = createCapsuleBone(
            j_calf_l_idx,
            j_foot_l_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r,
            rot);

        PxRigidDynamic* r_calf = createCapsuleBone(
            j_calf_r_idx,
            j_foot_r_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r,
            rot);

        PxRigidDynamic* l_arm = createCapsuleBone(
            j_upperarm_l_idx,
            j_lowerarm_l_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r);

        PxRigidDynamic* r_arm = createCapsuleBone(
            j_upperarm_r_idx,
            j_lowerarm_r_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r);

        PxRigidDynamic* l_forearm = createCapsuleBone(
            j_lowerarm_l_idx,
            j_hand_l_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r);

        PxRigidDynamic* r_forearm = createCapsuleBone(
            j_lowerarm_r_idx,
            j_hand_r_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r);

        PxRigidDynamic* l_hand = createSphereBone(
            j_middle_01_l_idx,
            *ragdoll,
            material,
            r,
            skeleton,
            model,
            1.0f,
            scale);

        PxRigidDynamic* r_hand = createSphereBone(
            j_middle_01_r_idx,
            *ragdoll,
            material,
            r,
            skeleton,
            model,
            1.0f,
            scale);

        rot = mat4::identity;

        PxRigidDynamic* l_foot = createCapsuleBone(
            j_foot_l_idx,
            j_ball_l_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r,
            rot);

        PxRigidDynamic* r_foot = createCapsuleBone(
            j_foot_r_idx,
            j_ball_r_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            r,
            rot);

        for (int i = 1; i < skeleton->joints.size(); i++)
        {
            uint32_t chosenIdx = 0;
            PxRigidDynamic* body = ragdoll->findRecentBody(i, skeleton, chosenIdx);

            PxTransform pxPose = body->getGlobalPose();
            mat4 bodyGlobalTransform = trsToMat4(trs{ createVec3(pxPose.p), createQuat(pxPose.q) });
            mat4 invBodyGlobalTransform = invert(bodyGlobalTransform);
            mat4 bindPoseWs = modelWithoutScale * invert(joints[i].invBindTransform);
            vec3 jointPosWs = bindPosWs(joints[i].invBindTransform, model) * scale;

            vec4 p = invBodyGlobalTransform * vec4(jointPosWs, 1.0f);
            ragdoll->relativeJointPoses[i] = vec3(p.x, p.y, p.z);
            ragdoll->originalBodyRotations[i] = mat4ToTRS(bodyGlobalTransform).rotation;

            if (ragdoll->rigidBodies[i])
            {
                // Rigid body position relative to the joint
                mat4 m = invert(model * invert(joints[i].invBindTransform));
                p = m * vec4(createVec3(ragdoll->rigidBodies[i]->getGlobalPose().p), 1.0f);

                ragdoll->bodyPosRelativeToJoint[i] = vec3(p.x, p.y, p.z);
                ragdoll->originalJointRotations[i] = mat4ToTRS(bindPoseWs).rotation;
            }

            finishBody(body, 1.0f, 1.0f);
        }

        //// ---------------------------------------------------------------------------------------------------------------
        //// Add rigid bodies to scene
        //// ---------------------------------------------------------------------------------------------------------------

        // Chest and Head
        scene->addActor(*pelvis);
        scene->addActor(*head);

        // Left Leg
        scene->addActor(*l_leg);
        scene->addActor(*l_calf);
        scene->addActor(*l_foot);

        // Right Leg
        scene->addActor(*r_leg);
        scene->addActor(*r_calf);
        scene->addActor(*r_foot);

        // Left Arm
        scene->addActor(*l_arm);
        scene->addActor(*l_forearm);
        scene->addActor(*l_hand);

        // Right Arm
        scene->addActor(*r_arm);
        scene->addActor(*r_forearm);
        scene->addActor(*r_hand);

        //// ---------------------------------------------------------------------------------------------------------------
        //// Create joints
        //// ---------------------------------------------------------------------------------------------------------------

        // Chest and Head
        createD6Joint(pelvis, head, skeleton, j_neck_01_idx, model, scale);

        // Chest to Thighs
        createD6Joint(pelvis, l_leg, skeleton, j_thigh_l_idx, model, scale);
        createD6Joint(pelvis, r_leg, skeleton, j_thigh_r_idx, model, scale);

        // Thighs to Calf
        createD6Joint(l_leg, l_calf, skeleton, j_calf_l_idx, model, scale);
        createD6Joint(r_leg, r_calf, skeleton, j_calf_r_idx, model, scale);
        //createRevoluteJoint(l_leg, l_calf, skeleton, j_calf_l_idx, mat4::identity, model);
        //createRevoluteJoint(r_leg, r_calf, skeleton, j_calf_r_idx, mat4::identity, model);

        // Calf to Foot
        createD6Joint(l_calf, l_foot, skeleton, j_foot_l_idx, model, scale);
        createD6Joint(r_calf, r_foot, skeleton, j_foot_r_idx, model, scale);

        // Chest to Upperarm
        createD6Joint(pelvis, l_arm, skeleton, j_upperarm_l_idx, model, scale);
        createD6Joint(pelvis, r_arm, skeleton, j_upperarm_r_idx, model, scale);

        //mat4 arm_rot = mat4::identity;
        //arm_rot = trsToMat4(trs{vec3(0.0f),  quat(vec3(0.0f, 0.0f, 1.0f), -PxPi / 2.0f) });

        // Upperarm to Lowerman
        //createRevoluteJoint(l_arm, l_forearm, skeleton, j_lowerarm_l_idx, arm_rot, model);
        createD6Joint(l_arm, l_forearm, skeleton, j_lowerarm_l_idx, model, scale);

        //arm_rot = mat4::identity;
        //arm_rot = trsToMat4(trs{ vec3(0.0f), quat(vec3(0.0f, 0.0f, 1.0f), PxPi / 2.0f) });
        createD6Joint(r_arm, r_forearm, skeleton, j_lowerarm_r_idx, model, scale);

        //createRevoluteJoint(r_arm, r_forearm, skeleton, j_lowerarm_r_idx, arm_rot, model);

        // Lowerman to Hand
        createD6Joint(l_forearm, l_hand, skeleton, j_hand_l_idx, model, scale);
        createD6Joint(r_forearm, r_hand, skeleton, j_hand_r_idx, model, scale);

        physics->update(0.016f);
    }

	std::vector<trs> px_ragdoll_component::apply(const mat4& modelScale, const mat4& modelRotation)
	{
		using namespace animation;

		if (ragdoll->rigidBodies.size() > 0)
		{
			std::vector<skeleton_joint>& joints = skeleton->joints;

			for (uint32_t i = 0; i < (size_t)limb_type_count; i++)
			{
				uint32_t chosenIdx = 1;
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
            //if(joints[idx].parentID != (uint32)-1)
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