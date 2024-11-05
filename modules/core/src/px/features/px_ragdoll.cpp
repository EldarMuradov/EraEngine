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

    static vec3 bindPosWs(mat4& bindPose, mat4& model)
    {
        mat4 m = model * bindPose;
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
        float hm = 1.0f,
        float mass = 1.0f)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;

        vec3 parentPos = bindPosWs(joints[parentIdx].bindTransform, model);
        vec3 childPos = bindPosWs(joints[childIdx].bindTransform, model);

        vec3 offset = parentPos - childPos;
        float len = length(offset);

        if (len - 2.0f * r > 0.1f)
            len = len - 2.0f * r;
        else
            len *= 0.9f;

        float halfHeight = hm * len / 2.0f;

        vec3 bodyPos = (parentPos + childPos) / 2.0f;

        PxShape* shape = physics->getPhysics()->createShape(PxCapsuleGeometry(r, halfHeight), *material);

        PxTransform local(createPxQuat(mat4ToTRS(rotation).rotation));
        shape->setLocalPose(local);
        shape->setSimulationFilterData(PxFilterData(-1, -1, 0, 0));
        shape->setQueryFilterData(PxFilterData(-1, -1, 0, 0));
        shape->setFlag(PxShapeFlag::eVISUALIZATION, true);
        shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);

        mat4 bindPose = joints[parentIdx].bindTransform;
        mat4 bindPoseWs = modelWithoutScale * bindPose;

        PxTransform px_transform(createPxVec3(bodyPos), createPxQuat(mat4ToTRS(bindPoseWs).rotation));

        PxRigidDynamic* body = physics->getPhysics()->createRigidDynamic(px_transform);
        body->setActorFlag(PxActorFlag::eVISUALIZATION, true);
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
        float hm = 1.0f,
        float mass = 1.0f)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;

        vec3 parentPos = (bindPosWs(joints[parentIdx].bindTransform, model) + offset);

        PxShape* shape = physics->getPhysics()->createShape(PxCapsuleGeometry(r, hm * l / 2.0f), *material);

        PxTransform local(createPxQuat(mat4ToTRS(rotation).rotation));
        shape->setLocalPose(local);
        shape->setFlag(PxShapeFlag::eVISUALIZATION, true);
        shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        shape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
        shape->setSimulationFilterData(PxFilterData(-1, -1, 0, 0));
        shape->setQueryFilterData(PxFilterData(-1, -1, 0, 0));
        mat4 invBindPose = joints[parentIdx].invBindTransform;
        mat4 bindPose = joints[parentIdx].bindTransform;
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
        float mass = 1.0f)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;

        vec3 parentPos = bindPosWs(joints[parentIdx].bindTransform, model);

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

    static PxD6Joint* createD6Joint(
        PxRigidDynamic* parent,
        PxRigidDynamic* child,
        ref<animation::animation_skeleton> skeleton,
        uint32_t jointPos,
        mat4 model = mat4::identity)
    {
        const auto& physics = physics_holder::physicsRef;

        auto& joints = skeleton->joints;
        vec3 p = bindPosWs(joints[jointPos].bindTransform, model);
        quat q = mat4ToTRS(joints[jointPos].bindTransform).rotation;

        PxD6Joint* joint = PxD6JointCreate(*physics->getPhysics(),
            parent,
            parent->getGlobalPose().transformInv(PxTransform(createPxVec3(p), createPxQuat(q))),
            child,
            child->getGlobalPose().transformInv(PxTransform(createPxVec3(p), createPxQuat(q))));

        joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

        return joint;
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
        vec3 p = bindPosWs(joints[jointPos].bindTransform, model);

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

    static constexpr float HEAD_MASS_PERCENTAGE = 0.0826f;
    static constexpr float BODY_UPPER_MASS_PERCENTAGE = 0.204f;
    static constexpr float BODY_LOWER_MASS_PERCENTAGE = 0.204f;
    static constexpr float ARM_MASS_PERCENTAGE = 0.07f;
    static constexpr float FOREARM_MASS_PERCENTAGE = 0.0467f;
    static constexpr float HAND_MASS_PERCENTAGE = 0.0065f;
    static constexpr float UP_LEG_MASS_PERCENTAGE = 0.085f;
    static constexpr float LEG_MASS_PERCENTAGE = 0.0475f;
    static constexpr float FOOT_MASS_PERCENTAGE = 0.014f;

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

        float mass = 100.0f;

        transforms.resize(joints.size());

        ragdoll->initialBodyPos.resize(joints.size());
        ragdoll->rigidBodies.resize(joints.size());
        ragdoll->childToParentJointDeltaPoses.resize(joints.size());
        ragdoll->childToParentJointDeltaRots.resize(joints.size());
        ragdoll->relativeJointPoses.resize(joints.size());
        ragdoll->originalBodyRotations.resize(joints.size());
        ragdoll->bodyPosRelativeToJoint.resize(joints.size());
        ragdoll->originalJointRotations.resize(joints.size());

        for (int i = 0; i < joints.size(); i++)
            ragdoll->rigidBodies[i] = nullptr;

        float r = 5.0f * scale;
        mat4 rot = mat4::identity;

        PxRigidDynamic* pelvis = createCapsuleBone(
            j_pelvis_idx,
            j_spine_03_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            15.0f * scale,
            rot,
            0.8f);
        finishBody(pelvis, mass * BODY_LOWER_MASS_PERCENTAGE, 1.0f);

        PxRigidDynamic* upperSpine = createCapsuleBone(
            j_spine_02_idx,
            j_neck_01_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            13.0f * scale,
            rot,
            0.4f);
        finishBody(upperSpine, mass * BODY_UPPER_MASS_PERCENTAGE, 1.0f);

        PxRigidDynamic* head = createCapsuleBone(
            j_head_idx,
            *ragdoll,
            material,
            vec3(0.0f, -5.0f * scale, 0.0f),
            4.0f * scale,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            6.0f * scale,
            rot);
        finishBody(head, mass * HEAD_MASS_PERCENTAGE, 1.0f);

        PxRigidDynamic* l_leg = createCapsuleBone(
            j_thigh_l_idx,
            j_calf_l_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            8.0f * scale,
            rot);
        finishBody(l_leg, mass * LEG_MASS_PERCENTAGE, 1.0f);

        PxRigidDynamic* r_leg = createCapsuleBone(
            j_thigh_r_idx,
            j_calf_r_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            8.0f * scale,
            rot);
        finishBody(r_leg, mass * LEG_MASS_PERCENTAGE, 1.0f);

        PxRigidDynamic* l_calf = createCapsuleBone(
            j_calf_l_idx,
            j_foot_l_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            7.0f * scale,
            rot);
        finishBody(l_calf, mass * LEG_MASS_PERCENTAGE, 1.0f);

        PxRigidDynamic* r_calf = createCapsuleBone(
            j_calf_r_idx,
            j_foot_r_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            7.0f * scale,
            rot);
        finishBody(r_calf, mass * LEG_MASS_PERCENTAGE, 1.0f);

        PxRigidDynamic* l_arm = createCapsuleBone(
            j_upperarm_l_idx,
            j_lowerarm_l_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            8.0f * scale);
        finishBody(l_arm, mass * ARM_MASS_PERCENTAGE, 1.0f);


        PxRigidDynamic* r_arm = createCapsuleBone(
            j_upperarm_r_idx,
            j_lowerarm_r_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            8.0f * scale);
        finishBody(r_arm, mass* ARM_MASS_PERCENTAGE, 1.0f);

        PxRigidDynamic* l_forearm = createCapsuleBone(
            j_lowerarm_l_idx,
            j_hand_l_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            7.0f * scale);
        finishBody(l_forearm, mass* ARM_MASS_PERCENTAGE, 1.0f);

        PxRigidDynamic* r_forearm = createCapsuleBone(
            j_lowerarm_r_idx,
            j_hand_r_idx,
            *ragdoll,
            material,
            skeleton,
            modelWithoutScale,
            modelOnlyScale,
            model,
            7.0f * scale);
        finishBody(r_forearm, mass* ARM_MASS_PERCENTAGE, 1.0f);

        /*PxRigidDynamic* l_hand = createSphereBone(
            j_middle_01_l_idx,
            *ragdoll,
            material,
            r,
            skeleton,
            model,
            1.0f);

        PxRigidDynamic* r_hand = createSphereBone(
            j_middle_01_r_idx,
            *ragdoll,
            material,
            r,
            skeleton,
            model,
            1.0f);*/

        rot = mat4::identity;

        /*PxRigidDynamic* l_foot = createCapsuleBone(
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
            rot);*/

        for (int i = 1; i < skeleton->joints.size(); i++)
        {
            uint32_t chosenIdx = 0;
            PxRigidDynamic* body = ragdoll->findRecentBody(i, skeleton, chosenIdx);

            PxTransform pxPose = body->getGlobalPose();
            trs bodyGlobalTransform = trs{ createVec3(pxPose.p), createQuat(pxPose.q) };

            mat4 bodyPosTransformMat = trsToMat4(bodyGlobalTransform);
            mat4 invBodyPosTransformMat = invert(bodyPosTransformMat);

            mat4 bindPosWS = modelWithoutScale * skeleton->joints[i].bindTransform;

            trs jointTrs = mat4ToTRS(skeleton->joints[i].bindTransform);

            ragdoll->initialBodyPos[i] = bodyGlobalTransform.position;
            ragdoll->relativeJointPoses[i] = (invBodyPosTransformMat * vec4(jointTrs.position, 1.0f)).xyz;
            ragdoll->originalBodyRotations[i] = normalize(bodyGlobalTransform.rotation);

            if (ragdoll->rigidBodies[i])
            {
                mat4 m = invert(model * skeleton->joints[i].bindTransform);
                ragdoll->bodyPosRelativeToJoint[i] = (m * vec4(bodyGlobalTransform.position, 1.0f)).xyz;
                ragdoll->originalJointRotations[i] = normalize(mat4ToTRS(bindPosWS).rotation);
            }

            ragdoll->childToParentJointDeltaPoses[i] = bodyGlobalTransform.position / scale - jointTrs.position;

            ragdoll->childToParentJointDeltaRots[i] = normalize(conjugate(jointTrs.rotation) * bodyGlobalTransform.rotation);
        }
        PxTransform pxPose = pelvis->getGlobalPose();
        trs bodyGlobalTransform = trs{ createVec3(pxPose.p), createQuat(pxPose.q) };
        ragdoll->rootTrs = bodyGlobalTransform;

        //// ---------------------------------------------------------------------------------------------------------------
        //// Add rigid bodies to scene
        //// ---------------------------------------------------------------------------------------------------------------

        // Chest and Head
        scene->addActor(*pelvis);
        scene->addActor(*head);
        scene->addActor(*upperSpine);

        // Left Leg
        scene->addActor(*l_leg);
        scene->addActor(*l_calf);
        //scene->addActor(*l_foot);

        // Right Leg
        scene->addActor(*r_leg);
        scene->addActor(*r_calf);
        //scene->addActor(*r_foot);

        // Left Arm
        scene->addActor(*l_arm);
        scene->addActor(*l_forearm);
        //scene->addActor(*l_hand);

        // Right Arm
        scene->addActor(*r_arm);
        scene->addActor(*r_forearm);
        //scene->addActor(*r_hand);

        //// ---------------------------------------------------------------------------------------------------------------
        //// Create joints
        //// ---------------------------------------------------------------------------------------------------------------

        // Chest and Head
        auto* jointHeadpelvis = createD6Joint(upperSpine, head, skeleton, j_neck_01_idx, model);
        configD6Joint(3.14f / 8.0f, 3.14f / 8.0f, -3.14f / 8.0f, 3.14f / 8.0f, jointHeadpelvis);

        auto* jointUpelvis = createD6Joint(upperSpine, pelvis, skeleton, j_spine_03_idx, model);
        configD6Joint(3.14f / 12.0f, 3.14f / 12.0f, -3.14f / 12.0f, 3.14f / 12.0f, jointUpelvis);

        // Chest to Thighs
        auto* jointLLegpelvis = createD6Joint(pelvis, l_leg, skeleton, j_thigh_l_idx, model);
        configD6Joint(3.14f / 6.0f, 3.14f / 6.0f, -3.14f / 6.0f, 3.14f / 6.0f, jointLLegpelvis);

        auto* jointRLegpelvis = createD6Joint(pelvis, r_leg, skeleton, j_thigh_r_idx, model);
        configD6Joint(3.14f / 6.0f, 3.14f / 6.0f, -3.14f / 6.0f, 3.14f / 6.0f, jointRLegpelvis);

        // Thighs to Calf
        auto* jointLLegCalf = createD6Joint(l_leg, l_calf, skeleton, j_calf_l_idx, model);
        configD6Joint(3.14f / 6.0f, 3.14f / 6.0f, -3.14f / 6.0f, 3.14f / 6.0f, jointLLegCalf);

        auto* jointRLegCalf = createD6Joint(r_leg, r_calf, skeleton, j_calf_r_idx, model);
        configD6Joint(3.14f / 6.0f, 3.14f / 6.0f, -3.14f / 6.0f, 3.14f / 6.0f, jointRLegCalf);
        ////createRevoluteJoint(l_leg, l_calf, skeleton, j_calf_l_idx, mat4::identity, model);
        ////createRevoluteJoint(r_leg, r_calf, skeleton, j_calf_r_idx, mat4::identity, model);

        // Calf to Foot
        ////createD6Joint(l_calf, l_foot, skeleton, j_foot_l_idx, model);
        ////createD6Joint(r_calf, r_foot, skeleton, j_foot_r_idx, model);

        // Chest to Upperarm
        auto* jointLArmPelvis = createD6Joint(upperSpine, l_arm, skeleton, j_upperarm_l_idx, model);
        configD6Joint(3.14f / 4.0f, 3.14f / 4.0f, -3.14f / 6.0f, 3.14f / 6.0f, jointLArmPelvis);
        auto* jointRArmPelvis = createD6Joint(upperSpine, r_arm, skeleton, j_upperarm_r_idx, model);
        configD6Joint(3.14f / 4.0f, 3.14f / 4.0f, -3.14f / 6.0f, 3.14f / 6.0f, jointRArmPelvis);

        ////mat4 arm_rot = mat4::identity;
        ////arm_rot = trsToMat4(trs{vec3(0.0f),  quat(vec3(0.0f, 0.0f, 1.0f), -PxPi / 2.0f) });

        // Upperarm to Lowerman
        ////createRevoluteJoint(l_arm, l_forearm, skeleton, j_lowerarm_l_idx, arm_rot, model);
        auto* jointLArmForearm = createD6Joint(l_arm, l_forearm, skeleton, j_lowerarm_l_idx, model);
        configD6Joint(3.14f / 4.0f, 3.14f / 4.0f, -3.14f / 6.0f, 3.14f / 6.0f, jointLArmForearm);

        ////arm_rot = mat4::identity;
        ////arm_rot = trsToMat4(trs{ vec3(0.0f), quat(vec3(0.0f, 0.0f, 1.0f), PxPi / 2.0f) });
        auto* jointRArmForearm = createD6Joint(r_arm, r_forearm, skeleton, j_lowerarm_r_idx, model);
        configD6Joint(3.14f / 4.0f, 3.14f / 4.0f, -3.14f / 6.0f, 3.14f / 6.0f, jointRArmForearm);


        ////createRevoluteJoint(r_arm, r_forearm, skeleton, j_lowerarm_r_idx, arm_rot, model);

        // Lowerman to Hand
        //createD6Joint(l_forearm, l_hand, skeleton, j_hand_l_idx, model);
        //createD6Joint(r_forearm, r_hand, skeleton, j_hand_r_idx, model);

        ragdoll->setKinematic(false);

        physics->update(0.016f);

        simulated = true;
    }

    void px_ragdoll_component::updateKinematic()
    {
        if(!ragdoll->kinematic)
            ragdoll->setKinematic(true);

        for (int i = 0; i < ragdoll->rigidBodies.size(); ++i)
        {
            // TODO
            PxRigidDynamic* rb = ragdoll->rigidBodies[i];
            if (rb)
            {
                vec3 pos = (model * trsToMat4(transforms[i]) * vec4(ragdoll->bodyPosRelativeToJoint[i], 1.0f)).xyz;

                quat jointRot = mat4ToTRS(modelWithoutScale * trsToMat4(transforms[i])).rotation;
                quat deltaJRotation = normalize(conjugate(ragdoll->originalJointRotations[i]) * jointRot);

                quat rot = normalize(ragdoll->originalBodyRotations[i] * deltaJRotation);

                rb->setGlobalPose(PxTransform{createPxVec3(pos), createPxQuat(rot)});
            }
        }
    }

	std::vector<trs> px_ragdoll_component::apply(const mat4& modelRotation)
	{
		using namespace animation;

		if (ragdoll->rigidBodies.size() > 0)
		{
			std::vector<skeleton_joint>& joints = skeleton->joints;

            transforms[0] = mat4ToTRS(joints[0].bindTransform);

			for (uint32_t i = 1; i < joints.size(); i++)
			{
				uint32_t chosenIdx = 0;
				PxRigidDynamic* body = ragdoll->findRecentBody(i, skeleton, chosenIdx);

                PxTransform pxPose = body->getGlobalPose();
                trs bodyGlobalTransform = trs{ createVec3(pxPose.p), createQuat(pxPose.q) };
                trs jointTrs = mat4ToTRS(skeleton->joints[i].bindTransform);

                quat deltaRotation = bodyGlobalTransform.rotation * conjugate(ragdoll->originalBodyRotations[i]);
                mat4 translationRot;
                
                if (chosenIdx == i)
                {
                    vec3 pos = jointTrs.position + (bodyGlobalTransform.position / scale - ragdoll->initialBodyPos[i] / scale);

                    translationRot = trsToMat4(trs{ pos, deltaRotation * jointTrs.rotation });
                }
                else
                {
                    auto& parent = joints[chosenIdx];
                    trs parentPos = mat4ToTRS(parent.bindTransform);

                    vec3 localPos = jointTrs.position - parentPos.position;

                    localPos = deltaRotation * localPos;

                    vec3 pos = localPos + parentPos.position + (bodyGlobalTransform.position / scale - ragdoll->initialBodyPos[i] / scale);

                    translationRot = trsToMat4(trs{ pos, deltaRotation * jointTrs.rotation  });
                }

                transforms[i] = mat4ToTRS(invert(modelWithoutScale) * translationRot);
			}
		}

		return transforms;
	}

    void px_ragdoll_component::updateMotion(trs* outLocalTransforms, trs* outRootMotion)
    {
        using namespace animation;

        for (size_t i = 0; i < skeleton->joints.size(); ++i)
        {
            outLocalTransforms[i] = trs::identity;
        }

        for (size_t i = 0; i < ragdoll->rigidBodies.size(); ++i)
        {
            if (!ragdoll->rigidBodies[i])
                continue;
            const skeleton_joint& joint = skeleton->joints[i];
            trs motion;
            PxRigidDynamic* rootBody = ragdoll->rigidBodies[i];

            PxTransform pxPose = rootBody->getGlobalPose();
            trs bodyGlobalTransform = trs{ createVec3(pxPose.p), createQuat(pxPose.q) };
            motion.position = bodyGlobalTransform.position / scale - ragdoll->initialBodyPos[i] / scale;
            motion.rotation = bodyGlobalTransform.rotation * conjugate(ragdoll->originalBodyRotations[i]);
            motion.scale = vec3(1.0f);

            outLocalTransforms[i] = motion;
        }

        trs rootMotion = trs::identity;

        if (outRootMotion)
        {
            uint32_t j_pelvis_idx = skeleton->nameToJointID["pelvis"];
            PxRigidDynamic* rootBody = ragdoll->rigidBodies[j_pelvis_idx];

            PxTransform pxPose = rootBody->getGlobalPose();
            trs bodyGlobalTransform = trs{ createVec3(pxPose.p), createQuat(pxPose.q) };
            rootMotion.position = bodyGlobalTransform.position / scale - ragdoll->rootTrs.position / scale;
            rootMotion.rotation = bodyGlobalTransform.rotation * conjugate(ragdoll->rootTrs.rotation);
            rootMotion.scale = vec3(1.0f);
            *outRootMotion = rootMotion;
            ragdoll->rootTrs = bodyGlobalTransform;
        }
        else
        {
            outLocalTransforms[0] = rootMotion * outLocalTransforms[0];
        }
    }

    void px_ragdoll_component::update(float dt)
    {
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

        kinematic = state;
	}

}