#include "pch.h"

#include "px_physics_engine.h"
#include <core/log.h>
#include <scene/scene.h>
#include <px/physics/px_collider_component.h>

#ifndef PX_PHYSX_STATIC_LIB
#define PX_PHYSX_STATIC_LIB
#endif

#pragma comment(lib, "PhysXCooking_64.lib")

#include <application.h>
#include <scene/scene.h>

px_physics_engine* px_physics_engine::engine = nullptr;
std::mutex px_physics_engine::sync;

CollisionContactCallback collision_callback;

CCDContactModification contact_modification;

SimulationFilterCallback simulation_filter_callback;

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
	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, px_allocator, defaultErrorCallback);

	if (!foundation)
		throw std::exception("Failed to create {PxFoundation}. Error in {PhysicsEngine} ctor.");
	pvd = PxCreatePvd(*foundation);

	PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);

	if (transport == NULL)
		throw std::exception("Failed to create {PxPvdTransport}. Error in {PhysicsEngine} ctor.");

	pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	toleranceScale.length = 1.0;
	toleranceScale.speed = 981;

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, toleranceScale, true, pvd);

	dispatcher = PxDefaultCpuDispatcherCreate(nbCPUDispatcherThreads);

	PxCudaContextManagerDesc cudaContextManagerDesc;

	cudaContextManager = PxCreateCudaContextManager(*foundation, cudaContextManagerDesc, PxGetProfilerCallback());

	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = dispatcher;
	sceneDesc.gpuDynamicsConfig = PxgDynamicsMemoryConfig();
	sceneDesc.cudaContextManager = cudaContextManager;
	sceneDesc.filterShader = contactReportFilterShader;
	sceneDesc.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
	sceneDesc.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
	sceneDesc.simulationEventCallback = &collision_callback;

#if PX_GPU_BROAD_PHASE
	sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
#else
	sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eABP;
#endif

	sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

#if PX_GPU_BROAD_PHASE
	sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
#endif

	sceneDesc.flags |= PxSceneFlag::eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS;
	sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
	sceneDesc.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;
	sceneDesc.flags |= PxSceneFlag::eDISABLE_CCD_RESWEEP;
	sceneDesc.ccdContactModifyCallback = &contact_modification;
	sceneDesc.filterCallback = &simulation_filter_callback;

	scene = physics->createScene(sceneDesc);
	
	PxPvdSceneClient* client = scene->getScenePvdClient();

	if (client)
	{
		client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	auto& cookingParams = PxCookingParams(toleranceScale);

	if(!PxInitExtensions(*physics, pvd))
		LOG_ERROR("Physics> Failed to initialize extensions.");

#if PX_GPU_BROAD_PHASE
	cookingParams.buildGPUData = true;
#endif
	cookingParams.suppressTriangleMeshRemapTable = true;
	cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
	cookingParams.meshPreprocessParams = PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

	cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, cookingParams);

	insertationCallback = &physics->getPhysicsInsertionCallback();

	if (pvd->isConnected())
		LOG_MESSAGE("Physics> PVD Connection enabled.");

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

	PX_RELEASE(physics)
	PX_RELEASE(cooking)
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
}

px_physics_engine::~px_physics_engine()
{
	if (!released)
		release();

	physics->release();
	delete physics;

	releaseActors();
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
	physics->scene->lockWrite();
	physics->scene->getTaskManager()->startSimulation();
	physics->scene->collide(std::max(dt, 1.0f / frameRate));
	physics->scene->fetchCollision(true);
	physics->scene->advance();
	physics->scene->fetchResults(true);
	PxU32 nbActiveActors;
	PxActor** activeActors = physics->scene->getActiveActors(nbActiveActors);

	auto scene = &app->scene.getCurrentScene();

	for (size_t i = 0; i < nbActiveActors; i++)
	{
		if (auto rd = activeActors[i]->is<PxRigidDynamic>())
		{
			entity_handle* handle = static_cast<entity_handle*>(activeActors[i]->userData);
			eentity* renderObject = new eentity(*handle, *scene);
			const auto transform = &renderObject->getComponent<transform_component>();

			const auto& pxt = rd->getGlobalPose();
			const auto& pos = pxt.p;
			const auto& rot = pxt.q.getConjugate();
			transform->position = vec3(pos.x, pos.y, pos.z);
			transform->rotation = quat(rot.x, rot.y, rot.z, rot.w);
		}
	}

#if PX_ENABLE_RAYCAST_CCD
	physics->raycastCCD->doRaycastCCD(true);
#endif

	physics->scene->getTaskManager()->stopSimulation();
	physics->scene->unlockWrite();
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
		auto entity = actor->entity;
		px_collider_component_base* coll = (px_collider_component_base*)entity->getComponentIfExists<px_sphere_collider_component>();
		if (!coll)
			coll = (px_collider_component_base*)entity->getComponentIfExists<px_box_collider_component>();
		if (!coll)
			coll = (px_collider_component_base*)entity->getComponentIfExists<px_capsule_collider_component>();
		if (!coll)
			coll = (px_collider_component_base*)entity->getComponentIfExists<px_triangle_mesh_collider_component>();
		if (!coll)
			coll = (px_collider_component_base*)entity->getComponentIfExists<px_bounding_box_collider_component>();

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
	//actor->getRigidActor()->release();
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
	sync.unlock();
}

px_raycast_info px_physics_engine::raycast(px_rigidbody_component* rb, const vec3& origin, const vec3& dir, int maxDist, int maxHits, PxHitFlags hitFlags)
{
	//TODO: approximate origin to object boundind box -> dist face

	auto pose = rb->getRigidActor()->getGlobalPose();

	PxRaycastBuffer hit;
	PxVec3 o(origin.x, origin.y, origin.z);
	PxVec3 d(dir.x, dir.y, dir.z);
	bool status = physics->scene->raycast(o, d, maxDist, hit, PxHitFlag::eDEFAULT);

	if (status)
	{
		uint32 nb = hit.getNbAnyHits();
		std::cout << "Hits: " << nb << "\n";

		const auto& hitInfo1 = hit.getAnyHit(0);

		auto actor = actors_map[hitInfo1.actor];

		if (actor != rb)
			return
			{
				actor,
				hitInfo1.distance,
				hit.getNbAnyHits(),
				vec3(hitInfo1.position.x, hitInfo1.position.y, hitInfo1.position.z)
			};
		else if (hit.getNbAnyHits() > 1)
		{
			const auto& hitInfo2 = hit.getAnyHit(1);

			actor = actors_map[hitInfo2.actor];

			return
			{
				actor,
				hitInfo2.distance,
				hit.getNbAnyHits(),
				vec3(hitInfo2.position.x, hitInfo2.position.y, hitInfo2.position.z)
			};
		}
	}

	return px_raycast_info();
}

void CCDContactModification::onCCDContactModify(PxContactModifyPair* const pairs, PxU32 count)
{
	UNUSED(pairs);

	for (size_t i = 0; i < count; i++)
	{
		physx::PxContactModifyPair& cp = pairs[i];

		PxRigidActor* actor1 = (PxRigidActor*)cp.actor[0];
		PxRigidActor* actor2 = (PxRigidActor*)cp.actor[1];

		auto rb1 = px_physics_engine::get()->actors_map[actor1];
		auto rb2 = px_physics_engine::get()->actors_map[actor2];

		rb1->onCollisionEnter(rb2);
		rb2->onCollisionEnter(rb1);
	}
}

void CollisionContactCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	UNUSED(pairHeader);

	for (size_t i = 0; i < nbPairs; i++)
	{
		const physx::PxContactPair& cp = pairs[i];

		if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			PxRigidActor* actor1 = pairHeader.actors[0];
			PxRigidActor* actor2 = pairHeader.actors[1];

			auto rb1 = px_physics_engine::get()->actors_map[actor1];
			auto rb2 = px_physics_engine::get()->actors_map[actor2];

			rb1->onCollisionEnter(rb2);
			rb2->onCollisionEnter(rb1);
		}
	}
}

PxTriangleMesh* px_triangle_mesh::createTriangleMesh(PxTriangleMeshDesc desc)
{
	try
	{
		auto adapter = px_physics_engine::get()->getPhysicsAdapter();
		if (desc.triangles.count > 0 && desc.isValid())
			return adapter->cooking->createTriangleMesh(desc, *adapter->insertationCallback);
	}
	catch (...)
	{
		LOG_ERROR("Physics> Failed to create physics triangle mesh");
	}
	return nullptr;
}
