#include "pch.h"

#include "px_physics_engine.h"
#include <core/log.h>
#include <scene/scene.h>
#include <px/physics/px_collider_component.h>
#include <px/features/cloth/px_clothing_factory.h>

#ifndef PX_PHYSX_STATIC_LIB
#define PX_PHYSX_STATIC_LIB
#endif

#pragma comment(lib, "PhysXCooking_64.lib")

#include <application.h>
#include <scene/scene.h>

px_physics_engine* px_physics_engine::engine = nullptr;
std::queue<collision_handling_data> px_physics_engine::collisionQueue;
std::mutex px_physics_engine::sync;

px_collision_contact_callback collisionCallback;

px_CCD_contact_modification contactModification;

physx::PxFilterFlags contactReportFilterShader(
	PxFilterObjectAttributes attributes0,
	PxFilterData filterData0,
	PxFilterObjectAttributes attributes1,
	PxFilterData filterData1,
	PxPairFlags& pairFlags,
	const void* constantBlock,
	PxU32 constantBlockSize)
{
	UNUSED(constantBlockSize);
	UNUSED(constantBlock);

	if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
		return physx::PxFilterFlag::eDEFAULT;
	}

	pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;

	pairFlags = physx::PxPairFlag::eNOTIFY_CONTACT_POINTS | physx::PxPairFlag::eDETECT_DISCRETE_CONTACT;

	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;

	pairFlags = physx::PxPairFlag::eSOLVE_CONTACT | physx::PxPairFlag::eDETECT_DISCRETE_CONTACT
		| physx::PxPairFlag::eNOTIFY_TOUCH_FOUND
		| physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS
		| physx::PxPairFlag::eNOTIFY_CONTACT_POINTS | PxPairFlag::eMODIFY_CONTACTS;
	return physx::PxFilterFlag::eNOTIFY;
}

px_physics::~px_physics()
{
	if (!released)
	{
		release();
		released = true;
	}
}

void px_physics::initialize()
{
	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocatorCallback, errorReporter);

	if (PxGetSuggestedCudaDeviceOrdinal(foundation->getErrorCallback()) < 0)
		throw std::exception("Failed to create {PxFoundation}. Error in {PhysicsEngine} ctor.");

	if (!foundation)
		throw std::exception("Failed to create {PxFoundation}. Error in {PhysicsEngine} ctor.");

	pvd = PxCreatePvd(*foundation);

#if PX_ENABLE_PVD
	PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);

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
	sceneDesc.cpuDispatcher = dispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.cudaContextManager = cudaContextManager;
	sceneDesc.staticStructure = PxPruningStructureType::eDYNAMIC_AABB_TREE;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

	sceneDesc.solverType = PxSolverType::eTGS;

	sceneDesc.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
	sceneDesc.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
	sceneDesc.simulationEventCallback = &collisionCallback;

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
#endif

#if PX_ENABLE_PVD
	if (pvd->isConnected())
		LOG_MESSAGE("Physics> PVD Connection enabled.");
#endif

#if PX_ENABLE_RAYCAST_CCD
	raycastCCD = new RaycastCCDManager(scene);
#endif

#if PX_VEHICLE
	if(!PxInitVehicleSDK(*physics))
		LOG_ERROR("Physics> Failed to initialize PxVehicleSDK.");

	PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, 1));
	PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
#endif
}

void px_physics::release()
{
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

void px_physics::releaseScene()
{
	PxU32 size;
	auto actors = scene->getActiveActors(size);
	scene->removeActors(actors, size);
}

px_physics_engine::px_physics_engine(application* application) noexcept
{
	app = application;
	physics = new px_physics();
	physics->initialize();
	allocator.initialize(MB(256));
}

px_physics_engine::~px_physics_engine()
{
	releaseActors();
	if (!released)
		release();

	allocator.reset(true);
	physics->release();

	RELEASE_PTR(physics)
}

void px_physics_engine::initialize(application* application)
{
	if (engine)
		throw std::exception("Failed to create {px_physics_engine}. Error in {px_physics_engine::initialize()}. {px_physics_engine} is already created.");
	sync.lock();
	px_physics_engine::engine = new px_physics_engine(application);
	sync.unlock();
}

void px_physics_engine::release()
{
	sync.lock();
	if (engine)
	{
		delete engine;
		engine = nullptr;
	}
	sync.unlock();
}

void px_physics_engine::start()
{
}

void px_physics_engine::update(float dt)
{
	float stepSize = 1.0f / frameRate;

	physics->scene->lockWrite();
	physics->scene->getTaskManager()->startSimulation();

	void* scratchMemBlock = allocator.allocate(MB(16), 16U, true);

#if PX_GPU_BROAD_PHASE
	physics->scene->simulate(stepSize, NULL, scratchMemBlock, MB(16));
#else
	physics->scene->collide(std::max(dt, stepSize), NULL, scratchMemBlock, MB(16));
	physics->scene->fetchCollision(true);
	physics->scene->advance();
#endif

	physics->scene->fetchResults(true);

	physics->scene->flushSimulation();
	physics->scene->fetchResultsParticleSystem();
	physics->scene->getTaskManager()->stopSimulation();
	physics->scene->unlockWrite();

	PxActor** activeActors = physics->scene->getActiveActors(nbActiveActors);

	auto scene = &app->scene.getCurrentScene();

	for (size_t i = 0; i < nbActiveActors; i++)
	{
		if (auto rb = activeActors[i]->is<PxRigidDynamic>())
		{
			entity_handle* handle = static_cast<entity_handle*>(activeActors[i]->userData);
			eentity renderObject = { *handle, &scene->registry };
			const auto transform = &renderObject.getComponent<transform_component>();

			const auto& pxt = rb->getGlobalPose();
			const auto& pos = pxt.p;
			const auto& rot = pxt.q.getConjugate();
			transform->position = createVec3(pos);
			transform->rotation = createQuat(rot);
		}
	}

#if PX_ENABLE_RAYCAST_CCD
	physics->raycastCCD->doRaycastCCD(true);
#endif

	allocator.reset();
}

void px_physics_engine::resetActorsVelocityAndInertia()
{
	physics->scene->lockWrite();
	PxU32 nbActiveActors;
	PxActor** activeActors = physics->scene->getActiveActors(nbActiveActors);

	for (size_t i = 0; i < nbActiveActors; i++)
	{
		if (auto rd = activeActors[i]->is<PxRigidDynamic>())
		{
			rd->setAngularVelocity(PxVec3(0.0f));
			rd->setLinearVelocity(PxVec3(0.0f));
		}
	}

	physics->scene->flushSimulation();
	physics->scene->unlockWrite();
}

void px_physics_engine::addActor(px_rigidbody_component* actor, PxRigidActor* ractor, bool addToScene)
{
	sync.lock();
	if(addToScene)
		physics->scene->addActor(*ractor);

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

		if(coll)
			physics->raycastCCD->registerRaycastCCDObject(r, coll->getShape());
	}
#endif

	actors.emplace(actor);
	actors_map.insert(std::make_pair(ractor, actor));
	sync.unlock();
}

void px_physics_engine::removeActor(px_rigidbody_component* actor)
{
	sync.lock();
	actors.erase(actor);
	actors_map.erase(actor->getRigidActor());
	physics->scene->removeActor(*actor->getRigidActor());
	sync.unlock();
}

px_physics_engine* px_physics_engine::get()
{
	return engine;
}

PxPhysics* px_physics_engine::getPhysics()
{
	return engine->physics->physics;
}

void px_physics_engine::releaseActors() noexcept
{
	sync.lock();
	actors.clear();
	actors_map.clear();
	physics->scene->flushSimulation();
	sync.unlock();
}

px_raycast_info px_physics_engine::raycast(px_rigidbody_component* rb, const vec3& dir, int maxDist, bool hitTriggers, uint32_t layerMask, int maxHits)
{
	const auto& pose = rb->getRigidActor()->getGlobalPose().p - PxVec3(0.0f, 1.5f, 0.0f);

	PX_SCENE_QUERY_SETUP(true);
	PxRaycastBuffer buffer;
	PxVec3 d = createPxVec3(dir);
	bool status = physics->scene->raycast(pose, d, maxDist, buffer, hitFlags, PxQueryFilterData(), &engine->physics->queryFilter);

	if (status)
	{
		uint32 nb = buffer.getNbAnyHits();
		std::cout << "Hits: " << nb << "\n";
		uint32 index = 0;
		if (nb > 1)
			index = 1;
		const auto& hitInfo1 = buffer.getAnyHit(index);

		auto actor = actors_map[hitInfo1.actor];

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

			actor = actors_map[hitInfo2.actor];

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

void px_CCD_contact_modification::onCCDContactModify(PxContactModifyPair* const pairs, PxU32 count)
{
	UNUSED(pairs);

	for (size_t i = 0; i < count; i++)
	{
		physx::PxContactModifyPair& cp = pairs[i];

		PxRigidActor* actor1 = (PxRigidActor*)cp.actor[0];
		PxRigidActor* actor2 = (PxRigidActor*)cp.actor[1];

		auto rb1 = px_physics_engine::get()->actors_map[actor1];
		auto rb2 = px_physics_engine::get()->actors_map[actor2];

		if (rb1 && rb2)
		{
			rb1->onCollisionEnter(rb2);
			rb2->onCollisionEnter(rb1);
		}
	}
}

void px_collision_contact_callback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	UNUSED(pairHeader);
	
	const physx::PxU32 bufferSize = PX_CONTACT_BUFFER_SIZE;
	physx::PxContactPairPoint contacts[bufferSize];
	for (physx::PxU32 i = 0; i < nbPairs; i++)
	{
		const physx::PxContactPair& cp = pairs[i];
		physx::PxU32 nbContacts = pairs[i].extractContacts(contacts, bufferSize);
		for (physx::PxU32 j = 0; j < nbContacts; j++)
		{
			physx::PxVec3 point = contacts[j].position;
			physx::PxVec3 impulse = contacts[j].impulse;
			physx::PxU32 internalFaceIndex0 = contacts[j].internalFaceIndex0;
			physx::PxU32 internalFaceIndex1 = contacts[j].internalFaceIndex1;
			UNUSED(point);
			UNUSED(impulse);
			UNUSED(internalFaceIndex0);
			UNUSED(internalFaceIndex1);
		}

		if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			auto r1 = pairHeader.actors[0]->is<PxRigidActor>();
			auto r2 = pairHeader.actors[1]->is<PxRigidActor>();
			if (!r1 || !r2)
				return;
			PxRigidActor* actor1 = r1;
			PxRigidActor* actor2 = r2;

			auto rb1 = px_physics_engine::get()->actors_map[actor1];
			auto rb2 = px_physics_engine::get()->actors_map[actor2];

			if (rb1 && rb2)
			{
				rb1->onCollisionEnter(rb2);
				rb2->onCollisionEnter(rb1);
				px_physics_engine::collisionQueue.emplace(rb1->handle, rb2->handle);
			}
		}
	}
}

NODISCARD PxTriangleMesh* px_triangle_mesh::createTriangleMesh(PxTriangleMeshDesc desc)
{
	try
	{
		auto adapter = px_physics_engine::get()->getPhysicsAdapter();
		if (desc.triangles.count > 0 && desc.isValid())
		{
			auto cookingParams = PxCookingParams(px_physics_engine::get()->getPhysicsAdapter()->toleranceScale);
#if PX_GPU_BROAD_PHASE
			cookingParams.buildGPUData = true;
#endif
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
