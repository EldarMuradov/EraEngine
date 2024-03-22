#include "pch.h"

#include "px_physics_engine.h"
#include <core/log.h>
#include <scene/scene.h>
#include <px/features/cloth/px_clothing_factory.h>
#include <px/physics/px_rigidbody_component.h>
#include <px/physics/px_collider_component.h>
#include <application.h>
#include <scene/scene.h>

#pragma comment(lib, "PhysXCooking_64.lib")

namespace
{
	physics::px_simulation_event_callback simulationEventCallback;

	physics::px_CCD_contact_modification contactModification;
}

static physx::PxFilterFlags contactReportFilterShader(
	physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
	physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
	physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize) noexcept
{
	UNUSED(constantBlockSize);
	UNUSED(constantBlock);

	if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
		return physx::PxFilterFlag::eDEFAULT;
	}

	if (physx::PxFilterObjectIsKinematic(attributes0) || physx::PxFilterObjectIsKinematic(attributes1))
		return physx::PxFilterFlag::eKILL;

	pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;
	pairFlags |= physx::PxPairFlag::eDETECT_CCD_CONTACT;
	pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
	pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
	pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
	pairFlags |= physx::PxPairFlag::ePOST_SOLVER_VELOCITY;
	pairFlags |= physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;
	pairFlags |= physx::PxPairFlag::eSOLVE_CONTACT;
	pairFlags |= physx::PxPairFlag::eDETECT_DISCRETE_CONTACT;

	return physx::PxFilterFlag::eDEFAULT;
}

static void clearColliderFromCollection(const physics::px_rigidbody_component* collider,
	physx::PxArray<physics::px_simulation_event_callback::colliders_pair>& collection) noexcept
{
	const auto c = &collection[0];
	for (int32 i = 0; i < collection.size(); i++)
	{
		const physics::px_simulation_event_callback::colliders_pair cc = c[i];
		if (cc.first == collider || cc.second == collider)
		{
			collection.remove(i--);
			if (collection.empty())
				break;
		}
	}
}

physics::px_physics_engine::px_physics_engine(application* application) noexcept
{
	app = application;
	allocator.initialize(MB(256));

	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocatorCallback, errorReporter);

	if (PxGetSuggestedCudaDeviceOrdinal(foundation->getErrorCallback()) < 0)
		throw ::std::exception("Failed to create {PxFoundation}. Error in {PhysicsEngine} ctor.");

	if (!foundation)
		throw ::std::exception("Failed to create {PxFoundation}. Error in {PhysicsEngine} ctor.");

	pvd = PxCreatePvd(*foundation);

#if PX_ENABLE_PVD

	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	if (transport == NULL)
		throw std::exception("Failed to create {PxPvdTransport}. Error in {PhysicsEngine} ctor.");

	pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

#endif
	toleranceScale.length = 1.0;
	toleranceScale.speed = 10.0;
	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, toleranceScale, true, pvd);

	if (!PxInitExtensions(*physics, pvd))
		LOG_ERROR("Physics> Failed to initialize extensions.");

	dispatcher = PxDefaultCpuDispatcherCreate(nbCPUDispatcherThreads);

	PxCudaContextManagerDesc cudaContextManagerDesc;
	cudaContextManagerDesc.graphicsDevice = dxContext.device.Get();
	cudaContextManager = PxCreateCudaContextManager(*foundation, cudaContextManagerDesc, &profilerCallback);

	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = gravity;
	sceneDesc.cudaContextManager = cudaContextManager;
	sceneDesc.cpuDispatcher = dispatcher;

	sceneDesc.filterShader = contactReportFilterShader;
	sceneDesc.staticStructure = PxPruningStructureType::eDYNAMIC_AABB_TREE;

	sceneDesc.solverType = PxSolverType::eTGS;

	sceneDesc.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
	sceneDesc.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;

#if PX_GPU_BROAD_PHASE
	sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
	sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
	sceneDesc.gpuMaxNumPartitions = 8;
	sceneDesc.gpuDynamicsConfig = PxgDynamicsMemoryConfig();
#else
	sceneDesc.broadPhaseType = physx::PxBroadPhaseType::ePABP;
#endif
	sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
	sceneDesc.flags |= PxSceneFlag::eDISABLE_CCD_RESWEEP;
	sceneDesc.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;
	sceneDesc.frictionType = PxFrictionType::eTWO_DIRECTIONAL;
	sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;

	sceneDesc.flags |= PxSceneFlag::eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS;
	sceneDesc.flags |= PxSceneFlag::eENABLE_ENHANCED_DETERMINISM;
	sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

	sceneDesc.simulationEventCallback = &simulationEventCallback;
	sceneDesc.filterCallback = &simulationFilterCallback;
	sceneDesc.ccdContactModifyCallback = &contactModification;

	scene = physics->createScene(sceneDesc);

#if PX_ENABLE_PVD
	PxPvdSceneClient* client = scene->getScenePvdClient();

	if (client)
	{
		client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	if (pvd->isConnected())
		LOG_MESSAGE("Physics> PVD Connection enabled.");
#endif

#if PX_ENABLE_RAYCAST_CCD
	raycastCCD = new RaycastCCDManager(scene);
#endif

#if PX_VEHICLE
	if (!PxInitVehicleSDK(*physics))
		LOG_ERROR("Physics> Failed to initialize PxVehicleSDK.");

	PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, 1));
	PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
#endif
}

physics::px_physics_engine::~px_physics_engine()
{
	if (!released)
		release();
}

void physics::px_physics_engine::release() noexcept
{
	lock lock{ sync };

	releaseActors();
	releaseScene();

#if PX_VEHICLE
	PxCloseVehicleSDK();
#endif

	PxCloseExtensions();

	PX_RELEASE(physics)
	PX_RELEASE(pvd)
	PX_RELEASE(foundation)
	PX_RELEASE(scene)
	PX_RELEASE(cudaContextManager)

#if PX_ENABLE_RAYCAST_CCD
	delete raycastCCD;
	raycastCCD = nullptr;
#endif

	released = true;
}

void physics::px_physics_engine::start() noexcept
{
}

void physics::px_physics_engine::update(float dt) noexcept
{
	const float stepSize = 1.0f / frameRate;

	scene->lockWrite();
	scene->getTaskManager()->startSimulation();

	void* scratchMemBlock = allocator.allocate(MB(16), 16U, true);

	scene->simulate(stepSize, NULL, scratchMemBlock, MB(16));

	scene->fetchResults(true);

	scene->flushSimulation();
	scene->fetchResultsParticleSystem();
	scene->getTaskManager()->stopSimulation();
	scene->unlockWrite();

	scene->lockRead();
	PxActor** activeActors = scene->getActiveActors(nbActiveActors);

	auto gameScene = app->getCurrentScene();

	for (size_t i = 0; i < nbActiveActors; i++)
	{
		if (auto rb = activeActors[i]->is<PxRigidDynamic>())
		{
			entity_handle* handle = static_cast<entity_handle*>(activeActors[i]->userData);
			eentity renderObject = { *handle, &gameScene->registry };
			const auto transform = &renderObject.getComponent<transform_component>();

			const auto& pxt = rb->getGlobalPose();
			const auto& pos = pxt.p;
			const auto& rot = pxt.q.getConjugate();
			transform->position = createVec3(pos);
			transform->rotation = createQuat(rot);
		}
	}

	scene->unlockRead();

#if PX_ENABLE_RAYCAST_CCD
	raycastCCD->doRaycastCCD(true);
#endif

	simulationEventCallback.sendCollisionEvents();
	simulationEventCallback.sendTriggerEvents();

	simulationEventCallback.clear();

	allocator.reset();
}

void physics::px_physics_engine::resetActorsVelocityAndInertia() noexcept
{
	scene->lockWrite();
	PxU32 nbActiveActors;
	PxActor** activeActors = scene->getActiveActors(nbActiveActors);

	for (size_t i = 0; i < nbActiveActors; i++)
	{
		if (auto rd = activeActors[i]->is<PxRigidDynamic>())
		{
			rd->setAngularVelocity(PxVec3(0.0f));
			rd->setLinearVelocity(PxVec3(0.0f));
		}
	}

	scene->flushSimulation();
	scene->unlockWrite();
}

void physics::px_physics_engine::addActor(px_rigidbody_component* actor, PxRigidActor* ractor, bool addToScene) noexcept
{
	scene->lockWrite();
	if (addToScene)
		scene->addActor(*ractor);

#if PX_ENABLE_RAYCAST_CCD
	if (auto r = ractor->is<PxRigidDynamic>())
	{
		auto handle = (entity_handle)actor->handle;
		auto scene = &app->scene.getCurrentScene();
		eentity entity = { handle, &scene->registry };
		px_collider_component_base* coll = (px_collider_component_base*)entity.getComponentIfExists<px_sphere_collider_component>();
		if (!coll)
			coll = (px_collider_component_base*)entity.getComponentIfExists<px_box_collider_component>();
		if (!coll)
			coll = (px_collider_component_base*)entity.getComponentIfExists<px_capsule_collider_component>();
		if (!coll)
			coll = (px_collider_component_base*)entity.getComponentIfExists<px_triangle_mesh_collider_component>();
		if (!coll)
			coll = (px_collider_component_base*)entity.getComponentIfExists<px_bounding_box_collider_component>();
		if (!coll)
			coll = (px_collider_component_base*)entity.getComponentIfExists<px_convex_mesh_collider_component>();

		if (coll)
			physics->raycastCCD->registerRaycastCCDObject(r, coll->getShape());
	}
#endif

	actors.emplace(actor);
	actorsMap.insert(::std::make_pair(ractor, actor));
	scene->unlockWrite();
}

void physics::px_physics_engine::removeActor(px_rigidbody_component* actor) noexcept
{
	scene->lockWrite();
	actors.erase(actor);
	actorsMap.erase(actor->getRigidActor());
	scene->removeActor(*actor->getRigidActor());
	scene->unlockWrite();
}

void physics::px_physics_engine::releaseActors() noexcept
{
	scene->lockWrite();
	auto gameScene = app->getCurrentScene();

	for (auto& actor : actors)
		actor->release();
	for (auto& coll : colliders)
		coll->release();

	actors.clear();
	actorsMap.clear();
	scene->flushSimulation();
	scene->unlockWrite();
}

void physics::px_physics_engine::releaseScene() noexcept
{
	PxU32 size;
	auto actors = scene->getActiveActors(size);
	scene->removeActors(actors, size);
}

physics::px_raycast_info physics::px_physics_engine::raycast(px_rigidbody_component* rb, const vec3& dir, int maxDist, bool hitTriggers, uint32_t layerMask, int maxHits)
{
	const auto& pose = rb->getRigidActor()->getGlobalPose().p - PxVec3(0.0f, 1.5f, 0.0f);

	PX_SCENE_QUERY_SETUP(true);
	PxRaycastBuffer buffer;
	PxVec3 d = createPxVec3(dir);
	bool status = scene->raycast(pose, d, maxDist, buffer, hitFlags, PxQueryFilterData(), &queryFilter);

	if (status)
	{
		uint32 nb = buffer.getNbAnyHits();
		::std::cout << "Hits: " << nb << "\n";
		uint32 index = 0;
		if (nb > 1)
			index = 1;
		const auto& hitInfo1 = buffer.getAnyHit(index);

		auto actor = actorsMap[hitInfo1.actor];

		if (actor != rb)
			return
		{
			actor,
			hitInfo1.distance,
			buffer.getNbAnyHits(),
			vec3(hitInfo1.position.x, hitInfo1.position.y, hitInfo1.position.z)
		};
		else if (buffer.getNbAnyHits() > 1)
		{
			const auto& hitInfo2 = buffer.getAnyHit(1);

			actor = actorsMap[hitInfo2.actor];

			return
			{
				actor,
				hitInfo2.distance,
				buffer.getNbAnyHits(),
				vec3(hitInfo2.position.x, hitInfo2.position.y, hitInfo2.position.z)
			};
		}
	}

	return px_raycast_info();
}

void physics::px_CCD_contact_modification::onCCDContactModify(PxContactModifyPair* const pairs, PxU32 count)
{
	UNUSED(pairs);

	for (size_t i = 0; i < count; i++)
	{
		physx::PxContactModifyPair& cp = pairs[i];

		// TODO
	}
}

void physics::px_simulation_event_callback::clear()
{
	newCollisions.clear();
	removedCollisions.clear();

	newTriggerPairs.clear();
	lostTriggerPairs.clear();
}

void physics::px_simulation_event_callback::sendCollisionEvents()
{
	for (auto& c : removedCollisions)
	{
		c.thisActor->onCollisionExit(c.otherActor);
		c.swapObjects();
		LOG_MESSAGE("VISHEL");
		c.thisActor->onCollisionExit(c.otherActor);
		c.swapObjects();
		physics::physics_holder::physicsRef->collisionExitQueue.emplace(c.thisActor->handle, c.otherActor->handle);
	}

	for (auto& c : newCollisions)
	{
		c.thisActor->onCollisionEnter(c.otherActor);
		c.swapObjects();
		LOG_MESSAGE("VOSHOL");
		c.thisActor->onCollisionEnter(c.otherActor);
		c.swapObjects();
		physics::physics_holder::physicsRef->collisionQueue.emplace(c.thisActor->handle, c.otherActor->handle);
	}
}

void physics::px_simulation_event_callback::sendTriggerEvents()
{
}

void physics::px_simulation_event_callback::onColliderRemoved(px_rigidbody_component* collider)
{
	clearColliderFromCollection(collider, newTriggerPairs);
	clearColliderFromCollection(collider, lostTriggerPairs);
}

void physics::px_simulation_event_callback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	UNUSED(pairHeader);

	const physx::PxU32 bufferSize = PX_CONTACT_BUFFER_SIZE;
	physx::PxContactPairPoint contacts[bufferSize];

	px_collision collision{};
	PxContactPairExtraDataIterator iter(pairHeader.extraDataStream, pairHeader.extraDataStreamSize);

	for (physx::PxU32 i = 0; i < nbPairs; i++)
	{
		const physx::PxContactPair& cp = pairs[i];
		physx::PxU32 nbContacts = pairs[i].extractContacts(contacts, bufferSize);

		const bool hasPostVelocities = !cp.flags.isSet(PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH);

		for (physx::PxU32 j = 0; j < nbContacts; j++)
		{
			physx::PxVec3 point = contacts[j].position;
			physx::PxVec3 impulse = contacts[j].impulse;
			physx::PxU32 internalFaceIndex0 = contacts[j].internalFaceIndex0;
			physx::PxU32 internalFaceIndex1 = contacts[j].internalFaceIndex1;

			collision.impulse += impulse;

			UNUSED(point);
			UNUSED(impulse);
			UNUSED(internalFaceIndex0);
			UNUSED(internalFaceIndex1);
		}

		collision.thisVelocity = collision.otherVelocity = PxVec3(0.0f);

		if (hasPostVelocities && iter.nextItemSet())
		{
			ASSERT(iter.contactPairIndex == i);
			if (iter.postSolverVelocity)
			{
				collision.thisVelocity = iter.postSolverVelocity->linearVelocity[0];
				collision.otherVelocity = iter.postSolverVelocity->linearVelocity[1];
			}
		}

		if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			auto r1 = pairHeader.actors[0]->is<PxRigidActor>();
			auto r2 = pairHeader.actors[1]->is<PxRigidActor>();

			if (!r1 || !r2)
				return;

			PxRigidActor* actor1 = r1;
			PxRigidActor* actor2 = r2;

			auto rb1 = physics::physics_holder::physicsRef->actorsMap[actor1];
			auto rb2 = physics::physics_holder::physicsRef->actorsMap[actor2];

			if (!rb1 || !rb2)
				return;

			collision.thisActor = rb1;
			collision.otherActor = rb2;

			if (cp.flags & PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
			{
				newCollisions.pushBack(collision);
			}
		}
		else if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
		{
			auto r1 = pairHeader.actors[0]->is<PxRigidActor>();
			auto r2 = pairHeader.actors[1]->is<PxRigidActor>();

			if (!r1 || !r2)
				return;

			PxRigidActor* actor1 = r1;
			PxRigidActor* actor2 = r2;

			auto rb1 = physics::physics_holder::physicsRef->actorsMap[actor1];
			auto rb2 = physics::physics_holder::physicsRef->actorsMap[actor2];

			if (!rb1 || !rb2)
				return;

			collision.thisActor = rb1;
			collision.otherActor = rb2;

			if (cp.flags & PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
			{
				removedCollisions.pushBack(collision);
			}
		}
	}
}

NODISCARD physx::PxTriangleMesh* physics::px_triangle_mesh_builder::createTriangleMesh(PxTriangleMeshDesc desc)
{
	try
	{
		if (desc.triangles.count > 0 && desc.isValid())
		{
			auto cookingParams = PxCookingParams(physics::physics_holder::physicsRef->getTolerancesScale());
#if PX_GPU_BROAD_PHASE
			cookingParams.buildGPUData = true;
#endif
			cookingParams.gaussMapLimit = 256;
			cookingParams.suppressTriangleMeshRemapTable = true;
			cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
			cookingParams.meshPreprocessParams = PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
			return PxCreateTriangleMesh(cookingParams, desc);
		}
	}
	catch (...)
	{
		LOG_ERROR("Physics> Failed to create physics triangle mesh");
	}
	return nullptr;
}

#if PX_VEHICLE

void physx::setupWheelsSimulationData(const PxF32 wheelMass, const PxF32 wheelMOI, const PxF32 wheelRadius, const PxF32 wheelWidth, const PxU32 numWheels, const PxVec3* wheelCenterActorOffsets, const PxVec3& chassisCMOffset, const PxF32 chassisMass, PxVehicleWheelsSimData* wheelsSimData)
{
	PxVehicleWheelData wheels[PX_MAX_NB_WHEELS];
	{
		for (PxU32 i = 0; i < numWheels; i++)
		{
			wheels[i].mMass = wheelMass;
			wheels[i].mMOI = wheelMOI;
			wheels[i].mRadius = wheelRadius;
			wheels[i].mWidth = wheelWidth;
		}

		wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxHandBrakeTorque = 4000.0f;
		wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxHandBrakeTorque = 4000.0f;
		wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxSteer = PxPi * 0.3333f;
		wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxSteer = PxPi * 0.3333f;
	}

	PxVehicleTireData tires[PX_MAX_NB_WHEELS];
	{
		for (PxU32 i = 0; i < numWheels; i++)
			tires[i].mType = TIRE_TYPE_NORMAL;
	}

	PxVehicleSuspensionData suspensions[PX_MAX_NB_WHEELS];
	{
		PxF32 suspSprungMasses[PX_MAX_NB_WHEELS];
		PxVehicleComputeSprungMasses
		(numWheels, wheelCenterActorOffsets,
			chassisCMOffset, chassisMass, 1, suspSprungMasses);
		for (PxU32 i = 0; i < numWheels; i++)
		{
			suspensions[i].mMaxCompression = 0.3f;
			suspensions[i].mMaxDroop = 0.1f;
			suspensions[i].mSpringStrength = 35000.0f;
			suspensions[i].mSpringDamperRate = 4500.0f;
			suspensions[i].mSprungMass = suspSprungMasses[i];
		}

		const PxF32 camberAngleAtRest = 0.0;
		const PxF32 camberAngleAtMaxDroop = 0.01f;
		const PxF32 camberAngleAtMaxCompression = -0.01f;
		for (PxU32 i = 0; i < numWheels; i += 2)
		{
			suspensions[i + 0].mCamberAtRest = camberAngleAtRest;
			suspensions[i + 1].mCamberAtRest = -camberAngleAtRest;
			suspensions[i + 0].mCamberAtMaxDroop = camberAngleAtMaxDroop;
			suspensions[i + 1].mCamberAtMaxDroop = -camberAngleAtMaxDroop;
			suspensions[i + 0].mCamberAtMaxCompression = camberAngleAtMaxCompression;
			suspensions[i + 1].mCamberAtMaxCompression = -camberAngleAtMaxCompression;
		}
	}

	PxVec3 suspTravelDirections[PX_MAX_NB_WHEELS];
	PxVec3 wheelCentreCMOffsets[PX_MAX_NB_WHEELS];
	PxVec3 suspForceAppCMOffsets[PX_MAX_NB_WHEELS];
	PxVec3 tireForceAppCMOffsets[PX_MAX_NB_WHEELS];
	{
		for (PxU32 i = 0; i < numWheels; i++)
		{
			suspTravelDirections[i] = PxVec3(0, -1, 0);

			wheelCentreCMOffsets[i] =
				wheelCenterActorOffsets[i] - chassisCMOffset;

			suspForceAppCMOffsets[i] =
				PxVec3(wheelCentreCMOffsets[i].x, -0.3f, wheelCentreCMOffsets[i].z);

			tireForceAppCMOffsets[i] =
				PxVec3(wheelCentreCMOffsets[i].x, -0.3f, wheelCentreCMOffsets[i].z);
		}
	}

	PxFilterData qryFilterData;
	setupNonDrivableSurface(qryFilterData);

	for (PxU32 i = 0; i < numWheels; i++)
	{
		wheelsSimData->setWheelData(i, wheels[i]);
		wheelsSimData->setTireData(i, tires[i]);
		wheelsSimData->setSuspensionData(i, suspensions[i]);
		wheelsSimData->setSuspTravelDirection(i, suspTravelDirections[i]);
		wheelsSimData->setWheelCentreOffset(i, wheelCentreCMOffsets[i]);
		wheelsSimData->setSuspForceAppPointOffset(i, suspForceAppCMOffsets[i]);
		wheelsSimData->setTireForceAppPointOffset(i, tireForceAppCMOffsets[i]);
		wheelsSimData->setSceneQueryFilterData(i, qryFilterData);
		wheelsSimData->setWheelShapeMapping(i, i);
	}
}

PxRigidDynamic* physx::createVehicleActor(const PxVehicleChassisData& chassisData, PxMaterial** wheelMaterials, PxConvexMesh** wheelConvexMeshes, const PxU32 numWheels, const PxFilterData& wheelSimFilterData, PxMaterial** chassisMaterials, PxConvexMesh** chassisConvexMeshes, const PxU32 numChassisMeshes, const PxFilterData& chassisSimFilterData, PxPhysics& physics)
{
	PxRigidDynamic* vehActor = physics.createRigidDynamic(PxTransform(PxIdentity));

	PxFilterData wheelQryFilterData;
	setupNonDrivableSurface(wheelQryFilterData);
	PxFilterData chassisQryFilterData;
	setupNonDrivableSurface(chassisQryFilterData);

	for (PxU32 i = 0; i < numWheels; i++)
	{
		PxConvexMeshGeometry geom(wheelConvexMeshes[i]);
		PxShape* wheelShape = PxRigidActorExt::createExclusiveShape(*vehActor, geom, *wheelMaterials[i]);
		wheelShape->setQueryFilterData(wheelQryFilterData);
		wheelShape->setSimulationFilterData(wheelSimFilterData);
		wheelShape->setLocalPose(PxTransform(PxIdentity));
	}

	for (PxU32 i = 0; i < numChassisMeshes; i++)
	{
		PxShape* chassisShape = PxRigidActorExt::createExclusiveShape(*vehActor, PxConvexMeshGeometry(chassisConvexMeshes[i]), *chassisMaterials[i]);
		chassisShape->setQueryFilterData(chassisQryFilterData);
		chassisShape->setSimulationFilterData(chassisSimFilterData);
		chassisShape->setLocalPose(PxTransform(PxIdentity));
	}

	vehActor->setMass(chassisData.mMass);
	vehActor->setMassSpaceInertiaTensor(chassisData.mMOI);
	vehActor->setCMassLocalPose(PxTransform(chassisData.mCMOffset, PxQuat(PxIdentity)));

	return vehActor;
}

PxVehicleDriveNW* physx::instantiate4WVersion(const PxVehicleDriveNW& vehicle18W, PxPhysics& physics)
{
	PxReal sprungMasses[4];
	{
		const PxReal rigidBodyMass = vehicle18W.getRigidDynamicActor()->getMass();
		const PxVec3 wheelCoords[4] =
		{
			vehicle18W.mWheelsSimData.getWheelCentreOffset(0),
			vehicle18W.mWheelsSimData.getWheelCentreOffset(1),
			vehicle18W.mWheelsSimData.getWheelCentreOffset(2),
			vehicle18W.mWheelsSimData.getWheelCentreOffset(3)
		};
		const PxU32 upDirection = 1;
		PxVehicleComputeSprungMasses(4, wheelCoords, PxVec3(0, 0, 0), rigidBodyMass, upDirection,
			sprungMasses);
	}

	PxVehicleWheelsSimData* wheelsSimData4W = PxVehicleWheelsSimData::allocate(4);
	for (PxU32 i = 0; i < 4; i++)
	{
		wheelsSimData4W->copy(vehicle18W.mWheelsSimData, i, i);

		PxVehicleSuspensionData suspData = wheelsSimData4W->getSuspensionData(i);
		suspData.setMassAndPreserveNaturalFrequency(sprungMasses[i]);
		wheelsSimData4W->setSuspensionData(i, suspData);
	}

	wheelsSimData4W->setTireLoadFilterData(vehicle18W.mWheelsSimData.getTireLoadFilterData());

	wheelsSimData4W->setWheelShapeMapping(0, 0);
	wheelsSimData4W->setWheelShapeMapping(1, 1);
	wheelsSimData4W->setWheelShapeMapping(2, 2);
	wheelsSimData4W->setWheelShapeMapping(3, 3);

	PxVehicleDriveSimDataNW driveSimData4W = vehicle18W.mDriveSimData;
	PxVehicleDifferentialNWData diff4W;
	diff4W.setDrivenWheel(0, true);
	diff4W.setDrivenWheel(1, true);
	diff4W.setDrivenWheel(2, true);
	diff4W.setDrivenWheel(3, true);
	driveSimData4W.setDiffData(diff4W);

	PxRigidDynamic* rigidDynamic =
		const_cast<PxRigidDynamic*>(vehicle18W.getRigidDynamicActor());
	PxVehicleDriveNW* vehicle4W =
		PxVehicleDriveNW::create(&physics, rigidDynamic, *wheelsSimData4W, driveSimData4W, 4);

	wheelsSimData4W->free();

	return vehicle4W;
}

void physx::swapToLowLodVersion(const PxVehicleDriveNW& vehicle18W, PxVehicleDrive4W* vehicle4W, PxVehicleWheels** vehicles, PxU32 vehicleId)
{
	vehicles[vehicleId] = vehicle4W;

	PxVehicleCopyDynamicsMap wheelMap;
	wheelMap.sourceWheelIds[0] = 0;
	wheelMap.sourceWheelIds[1] = 1;
	wheelMap.sourceWheelIds[2] = 2;
	wheelMap.sourceWheelIds[3] = 3;
	wheelMap.targetWheelIds[0] = 0;
	wheelMap.targetWheelIds[1] = 1;
	wheelMap.targetWheelIds[2] = 2;
	wheelMap.targetWheelIds[3] = 3;

	PxVehicleCopyDynamicsData(wheelMap, vehicle18W, vehicle4W);
}

void physx::swapToHighLowVersion(const PxVehicleDriveNW& vehicle4W, PxVehicleDrive4W* vehicle18W, PxVehicleWheels** vehicles, PxU32 vehicleId)
{
	vehicles[vehicleId] = vehicle18W;

	PxVehicleCopyDynamicsMap wheelMap;
	wheelMap.sourceWheelIds[0] = 0;
	wheelMap.sourceWheelIds[1] = 1;
	wheelMap.sourceWheelIds[2] = 2;
	wheelMap.sourceWheelIds[3] = 3;
	wheelMap.targetWheelIds[0] = 0;
	wheelMap.targetWheelIds[1] = 1;
	wheelMap.targetWheelIds[2] = 2;
	wheelMap.targetWheelIds[3] = 3;

	PxVehicleCopyDynamicsData(wheelMap, vehicle4W, vehicle18W);
}

#endif

NODISCARD physx::PxConvexMesh* physics::px_convex_mesh_builder::createConvexMesh(PxConvexMeshDesc desc)
{
	try
	{
		if (desc.isValid())
		{
			auto cookingParams = PxCookingParams(physics::physics_holder::physicsRef->getTolerancesScale());
#if PX_GPU_BROAD_PHASE
			cookingParams.buildGPUData = true;
#endif
			cookingParams.convexMeshCookingType = PxConvexMeshCookingType::eQUICKHULL;
			cookingParams.gaussMapLimit = 256;
			cookingParams.suppressTriangleMeshRemapTable = true;
			cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
			cookingParams.meshPreprocessParams = PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
			return PxCreateConvexMesh(cookingParams, desc);
		}
	}
	catch (...)
	{
		LOG_ERROR("Physics> Failed to create physics triangle mesh");
	}
	return nullptr;
}