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

ESGS_CollisionContactCallback collision_callback;

ESGS_CCDContactModification contact_modification;

ESGS_SimulationFilterCallback simulation_filter_callback;

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

	dispatcher = physx::PxDefaultCpuDispatcherCreate(4);

	PxCudaContextManagerDesc cudaContextManagerDesc;

	cudaContextManager = PxCreateCudaContextManager(*foundation, cudaContextManagerDesc, PxGetProfilerCallback());

	PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = dispatcher;
	sceneDesc.cudaContextManager = cudaContextManager;
	sceneDesc.filterShader = contactReportFilterShader;
	sceneDesc.kineKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
	sceneDesc.staticKineFilteringMode = physx::PxPairFilteringMode::eKEEP;
	sceneDesc.simulationEventCallback = &collision_callback;
	sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
	sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
	sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
	sceneDesc.flags |= PxSceneFlag::eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS;
	sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
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

	cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(toleranceScale));

	insertationCallback = &physics->getPhysicsInsertionCallback();

	if (pvd->isConnected())
		LOG_ERROR("Physics> PVD Connection enabled.");
}

void px_physics::release()
{
	PX_RELEASE(physics)
	PX_RELEASE(cooking)
	PX_RELEASE(pvd)
	PX_RELEASE(foundation)
	PX_RELEASE(scene)

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
	physics->scene->collide(std::max(dt, 1.0f / frameRate));
	physics->scene->fetchCollision(true);
	physics->scene->advance();
	physics->scene->fetchResults(true);

	PxU32 nbActiveActors;
	PxActor** activeActors = physics->scene->getActiveActors(nbActiveActors);

	auto scene = &app->scene.getCurrentScene();

	for (size_t i = 0; i < nbActiveActors; i++)
	{
		entity_handle* handle = static_cast<entity_handle*>(activeActors[i]->userData);
		eentity* renderObject = new eentity(*handle, *scene);
		const auto transform = &renderObject->getComponent<transform_component>();
		const auto& pxt = activeActors[i]->is<PxRigidDynamic>()->getGlobalPose();
		const auto& pos = pxt.p;
		const auto& rot = pxt.q.getConjugate();
		transform->position = vec3(pos.x, pos.y, pos.z);
		transform->rotation = quat(rot.x, rot.y, rot.z, rot.w);
	}
}

void px_physics_engine::addActor(px_rigidbody_component* actor, PxRigidActor* ractor)
{
	sync.lock();
	physics->scene->addActor(*ractor);
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
	auto collider = &rb->entity->getComponent<px_collider_component_base>();

	if (collider)
	{
		auto pose = rb->getRigidActor()->getGlobalPose();

		PxRaycastBuffer hit;
		PxVec3 o(origin.x, origin.y, origin.z);
		PxVec3 d(dir.x, dir.y, dir.z);
		bool status = physics->scene->raycast(o, d, maxDist, hit);

		if (status)
		{
			auto hitInfo = hit.getAnyHit(0);

			auto actor = actors_map[hitInfo.actor];

			if (actor != rb)
				return
				{
					actor,
					hitInfo.distance,
					hit.getNbAnyHits(),
					vec3(hitInfo.position.x, hitInfo.position.y, hitInfo.position.z)
				};
			else if (hit.getNbAnyHits() > 1)
			{
				hitInfo = hit.getAnyHit(1);

				actor = actors_map[hitInfo.actor];

				return
				{
					actor,
					hitInfo.distance,
					hit.getNbAnyHits(),
					vec3(hitInfo.position.x, hitInfo.position.y, hitInfo.position.z)
				};
			}
		}
	}

	return px_raycast_info();
}

void ESGS_CCDContactModification::onCCDContactModify(PxContactModifyPair* const pairs, PxU32 count)
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

void ESGS_CollisionContactCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
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
