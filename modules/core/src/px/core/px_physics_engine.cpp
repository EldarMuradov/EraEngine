// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "px/core/px_physics_engine.h"
#include "px/physics/px_rigidbody_component.h"
#include "px/physics/px_collider_component.h"
#include "px/physics/px_soft_body.h"
#include "px/features/px_vehicles.h"
#include "px/temp/px_mesh_generator.h"
#include "px/core/px_stepper.h"
#include "px/blast/px_blast_destructions.h"
#include "px/features/cloth/px_clothing_factory.h"
		 
#include "scene/scene.h"
#include "scene/scene.h"
		 
#include "application.h"

#include "omnipvd/PxOmniPvd.h"
#include "pvdruntime/include/OmniPvdWriter.h"
#include "pvdruntime/include/OmniPvdFileWriteStream.h"

#pragma comment(lib, "PhysXCooking_64.lib")

namespace era_engine
{
	static physx::PxOmniPvd* omniPvd = NULL;
	const char* omniPvdPath = "E:/OmniPVD/out.ovd";

	physics::px_CCD_contact_modification contactModification;

	static physx::PxFilterFlags contactReportFilterShader(
		physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
	{
		UNUSED(constantBlockSize);
		UNUSED(constantBlock);

		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		{
			pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;
			pairFlags |= physx::PxPairFlag::eDETECT_CCD_CONTACT;
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
			pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
			pairFlags |= physx::PxPairFlag::ePOST_SOLVER_VELOCITY;
			pairFlags |= physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;
			pairFlags |= physx::PxPairFlag::eSOLVE_CONTACT;
			pairFlags |= physx::PxPairFlag::eDETECT_DISCRETE_CONTACT;
		}
		
		return physx::PxFilterFlag::eDEFAULT;
	}

	static void clearColliderFromCollection(const physics::px_body_component* collider,
		physx::PxArray<physics::px_simulation_event_callback::colliders_pair>& collection)
	{
		const auto c = &collection[0];
		for (uint32 i = 0; i < collection.size(); i++)
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
}

namespace era_engine::physics
{
	concurrent_event_queue<blast_fracture_event> blastFractureQueue;

	era_engine::physics::px_fixed_stepper stepper;

	std::vector<PxFilterData> getFilterData(PxRigidActor* actor)
	{
		std::vector<PxShape*> shapes(actor->getNbShapes(), nullptr);
		std::vector<PxFilterData> out(shapes.size());

		actor->getShapes(&shapes[0], shapes.size());

		for (int i = 0; i < shapes.size(); ++i)
			out[i] = shapes[i]->getSimulationFilterData();

		return out;
	}

	bool isTrigger(const PxFilterData& data)
	{
		if (data.word0 != 0xffffffff)
			return false;
		if (data.word1 != 0xffffffff)
			return false;
		if (data.word2 != 0xffffffff)
			return false;
		if (data.word3 != 0xffffffff)
			return false;
		return true;
	}

	void setFilterData(PxRigidActor* actor, const std::vector<PxFilterData>& filterData)
	{
		std::vector<PxShape*> shapes(actor->getNbShapes(), nullptr);

		actor->getShapes(&shapes[0], shapes.size());
		for (int i = 0; i < shapes.size(); ++i)
			shapes[i]->setSimulationFilterData(filterData[i]);
	}

	PxVec3 px_collision::getRelativeVelocity() const
	{
		return thisVelocity - otherVelocity;
	}

	void px_collision::swapObjects()
	{
		if (!thisActor || !otherActor)
			return;
		std::swap(thisActor, otherActor);
		std::swap(thisVelocity, otherVelocity);
	}

	void px_error_reporter::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		if (message)
		{
			LOG_ERROR("PhysX Error> %s %s %u %s %s %s %u", message, "Code:", static_cast<int32>(code), "Source:", file, ":", line);

			std::ostringstream stream;
			stream << message;
			std::cout << stream.str() << "\n";
		}
		else
			std::cerr << "PhysX Error! \n";
	}

	void* px_profiler_callback::zoneStart(const char* eventName, bool detached, uint64_t contextId)
	{
#if ENABLE_CPU_PROFILING
		recordProfileEvent(profile_event_begin_block, eventName);
#endif

		LOG_MESSAGE("[%s] %s", eventName, "started");
		return nullptr;
	}

	void px_profiler_callback::zoneEnd(void* profilerData, const char* eventName, bool detached, uint64_t contextId)
	{
		LOG_MESSAGE("[%s] %s", eventName, "finished");

#if ENABLE_CPU_PROFILING
		recordProfileEvent(profile_event_end_block, eventName);
#endif
	}

	PxShape* px_character_controller_filter_callback::getShape(const PxController& controller)
	{
		PxRigidDynamic* actor = controller.getActor();

		if (!actor || actor->getNbShapes() < 1)
			return nullptr;

		PxShape* shape = nullptr;
		actor->getShapes(&shape, 1);

		return shape;
	}

	bool px_character_controller_filter_callback::filter(const PxController& a, const PxController& b)
	{
		PxShape* shapeA = getShape(a);
		if (!shapeA)
			return false;

		PxShape* shapeB = getShape(b);
		if (!shapeB)
			return false;

		if (PxFilterObjectIsTrigger(shapeB->getFlags()))
			return false;

		const PxFilterData shapeFilterA = shapeA->getQueryFilterData();
		const PxFilterData shapeFilterB = shapeB->getQueryFilterData();

		return shapeFilterA.word0 & shapeFilterB.word1;
	}

	PxQueryHitType::Enum px_query_filter::preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags)
	{
		if (!shape)
			return PxQueryHitType::eNONE;

		const PxFilterData shapeFilter = shape->getQueryFilterData();
		if ((filterData.word0 & shapeFilter.word0) == 0)
			return PxQueryHitType::eNONE;

		const bool hitTriggers = filterData.word2 != 0;
		if (!hitTriggers && shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
			return PxQueryHitType::eNONE;

		const bool blockSingle = filterData.word1 != 0;
		return blockSingle ? PxQueryHitType::eBLOCK : PxQueryHitType::eTOUCH;
	}

	PxQueryHitType::Enum px_query_filter::postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor)
	{
		return PxQueryHitType::eNONE;
	}

}

namespace era_engine
{
	physics::px_physics_engine::px_physics_engine()
	{
		allocator.initialize(MB(256U));

		foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocatorCallback, errorReporter);

		if (!foundation)
			throw std::exception("Failed to create {PxFoundation}. Error in {PhysicsEngine} ctor.");

#if PX_GPU_BROAD_PHASE
		if (PxGetSuggestedCudaDeviceOrdinal(foundation->getErrorCallback()) < 0)
			throw std::exception("Failed to create {PxFoundation}. Error in {PhysicsEngine} ctor.");
#endif

		pvd = PxCreatePvd(*foundation);

#if PX_ENABLE_PVD

		omniPvd = PxCreateOmniPvd(*foundation);
		if (!omniPvd)
		{
			printf("Error : could not create PxOmniPvd!");
			return;
		}
		OmniPvdWriter* omniWriter = omniPvd->getWriter();
		if (!omniWriter)
		{
			printf("Error : could not get an instance of PxOmniPvdWriter!");
			return;
		}
		OmniPvdFileWriteStream* fStream = omniPvd->getFileWriteStream();
		if (!fStream)
		{
			printf("Error : could not get an instance of PxOmniPvdFileWriteStream!");
			return;
		}
		fStream->setFileName(omniPvdPath);
		omniWriter->setWriteStream(static_cast<OmniPvdWriteStream&>(*fStream));

#endif

		toleranceScale.length = 1.0f;
		toleranceScale.speed = 9.81f;
		physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, toleranceScale, true, pvd, omniPvd);

		if (!PxInitExtensions(*physics, pvd))
			LOG_ERROR("Physics> Failed to initialize extensions.");

		defaultMaterial = physics->createMaterial(0.7f, 0.7f, 0.8f);

#if PX_ENABLE_PVD
		if (physics->getOmniPvd())
		{
			ASSERT(omniPvd->startSampling());
		}

		/*PxPvdSceneClient* client = scene->getScenePvdClient();

		if (client)
		{
			client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}

		if (pvd->isConnected())
			std::cout << "Physics> PVD Connection enabled.\n";*/
#endif

		dispatcher = PxDefaultCpuDispatcherCreate(nbCPUDispatcherThreads);

		PxSceneDesc sceneDesc(physics->getTolerancesScale());
		sceneDesc.gravity = gravity;
		sceneDesc.cpuDispatcher = dispatcher;
		sceneDesc.solverType = PxSolverType::eTGS;
		sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
		sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
		sceneDesc.flags |= PxSceneFlag::eENABLE_ENHANCED_DETERMINISM;
		sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
		sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
#if PX_GPU_BROAD_PHASE
		PxCudaContextManagerDesc cudaContextManagerDesc;
		cudaContextManagerDesc.graphicsDevice = dxContext.device.Get();
		cudaContextManager = PxCreateCudaContextManager(*foundation, cudaContextManagerDesc, &profilerCallback);
		sceneDesc.cudaContextManager = cudaContextManager;
		sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
		sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
		sceneDesc.gpuMaxNumPartitions = 8;
#else
		sceneDesc.broadPhaseType = physx::PxBroadPhaseType::ePABP;
#endif
		sceneDesc.filterShader = contactReportFilterShader;

		simulationEventCallback = make_ref<px_simulation_event_callback>();

		sceneDesc.simulationEventCallback = simulationEventCallback.get();

		scene = physics->createScene(sceneDesc);

		scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
		scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);
		scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);

		scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);

#if PX_ENABLE_RAYCAST_CCD
		raycastCCD = new RaycastCCDManager(scene);
#endif

#if PX_VEHICLE
		if (!PxInitVehicleSDK(*physics))
			LOG_ERROR("Physics> Failed to initialize PxVehicleSDK.");

		PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, 1));
		PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);

		//bool res = initVehicle(this);

		//if(!res)
			//LOG_ERROR("Physics> Failed to initialize PxVehicles.");
#endif
	}

	physics::px_physics_engine::~px_physics_engine()
	{
		if (!released)
			release();
	}

	void physics::px_physics_engine::release()
	{
		if (!released)
		{
			shared_spin_lock lock{ sync };

#if PX_VEHICLE
			PxCloseVehicleSDK();
			//cleanupVehicle();
#endif

			PxCloseExtensions();

			if (scene)
				scene->flushSimulation();

			PX_RELEASE(defaultMaterial)
			PX_RELEASE(scene)
			PX_RELEASE(dispatcher)
			PX_RELEASE(pvd)
			PX_RELEASE(omniPvd);
			PX_RELEASE(physics)
			PX_RELEASE(cudaContextManager)
			PX_RELEASE(foundation)

#if PX_ENABLE_RAYCAST_CCD
			delete raycastCCD;
			raycastCCD = nullptr;
#endif

			released = true;
		}
	}

	void physics::px_physics_engine::start()
	{
	}

	void physics::px_physics_engine::update(float dt)
	{
		startSimulation(dt);
		endSimulation();
	}

	void physics::px_physics_engine::startSimulation(float dt)
	{
		recordProfileEvent(profile_event_begin_block, "PhysX update");

		const float stepSize = 1.0f / frameRate;

		static constexpr uint64 align = 16U;

		static constexpr uint64 scratchMemBlockSize = MB(32U);

		static void* scratchMemBlock = allocator.allocate(scratchMemBlockSize, align, true);

#if PX_GPU_BROAD_PHASE

		{
			physics_lock_write lock{};

			scene->simulate(stepSize, NULL, scratchMemBlock, scratchMemBlockSize);
			scene->fetchResults(true);
		}

#else

		stepper.setup(stepSize);

		if (!stepper.advance(scene, stepSize, scratchMemBlock, scratchMemBlockSize))
			return;

		stepper.renderDone();

#endif
	}

	void physics::px_physics_engine::endSimulation()
	{
#if !PX_GPU_BROAD_PHASE
		stepper.wait(scene);
#endif
		{
			CPU_PROFILE_BLOCK("PhysX process simulation event callbacks steps");
			processSimulationEventCallbacks();
		}

		{
			CPU_PROFILE_BLOCK("PhysX sync transforms steps");
			syncTransforms();
		}

		recordProfileEvent(profile_event_end_block, "PhysX update");
	}

	void physics::px_physics_engine::resetActorsVelocityAndInertia()
	{
		physics_lock_write lock{};
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
	}

	void physics::px_physics_engine::addActor(px_body_component* actor, PxRigidActor* ractor, bool addToScene)
	{
		physics_lock_write lock{};
		if (addToScene)
			scene->addActor(*ractor);

#if PX_ENABLE_RAYCAST_CCD
		if (auto r = ractor->is<PxRigidDynamic>())
		{
			PxShape* shape = nullptr;

			r->getShapes(&shape, 1);

			raycastCCD->registerRaycastCCDObject(r, shape);
		}
#endif

		actors.emplace(actor);
		actorsMap.insert(::std::make_pair(ractor, actor));
	}

	void physics::px_physics_engine::removeActor(px_body_component* actor)
	{
		physics_lock_write lock{};
		actors.erase(actor);
		actorsMap.erase(actor->getRigidActor());
		scene->removeActor(*actor->getRigidActor());
	}

	void physics::px_physics_engine::releaseActors()
	{
		physics_lock_write lock{};
		auto gameScene = globalApp.getCurrentScene();

		for (auto& pair : collidersMap)
		{
			for (auto& coll : pair.second)
			{
				coll->release();
			}
		}

		for (auto& actor : actors)
			actor->release();

		actors.clear();
		actorsMap.clear();
		collidersMap.clear();
		scene->flushSimulation();
	}

	void physics::px_physics_engine::releaseScene()
	{
		PxU32 size;
		auto actors = scene->getActiveActors(size);
		scene->removeActors(actors, size);
	}

	void physics::px_physics_engine::explode(const vec3& worldPos, float damageRadius, float explosiveImpulse)
	{
		PxVec3 pos = createPxVec3(worldPos);
		px_explode_overlap_callback overlapCallback(pos, damageRadius, explosiveImpulse);
		scene->overlap(PxSphereGeometry(damageRadius), PxTransform(pos), overlapCallback);
	}

	physics::px_raycast_info physics::px_physics_engine::raycast(px_body_component* rb, const vec3& dir, int maxDist, bool hitTriggers, uint32_t layerMask, int maxHits)
	{
		const auto& pose = rb->getRigidActor()->getGlobalPose().p - PxVec3(0.0f, 1.5f, 0.0f);

		PX_SCENE_QUERY_SETUP(true);
		PxRaycastBuffer buffer;
		PxVec3 d = createPxVec3(dir);
		bool status = scene->raycast(pose, d, maxDist, buffer, hitFlags, PxQueryFilterData(), &queryFilter);

		if (status)
		{
			uint32 nb = buffer.getNbAnyHits();
			std::cout << "Hits: " << nb << "\n";
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

	bool physics::px_physics_engine::checkBox(const vec3& center, const vec3& halfExtents, const quat& rotation, bool hitTriggers, uint32 layerMask)
	{
		PX_SCENE_QUERY_SETUP_CHECK();
		const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
		const PxBoxGeometry geometry(createPxVec3(halfExtents));

		return scene->overlap(geometry, pose, buffer, filterData, &queryFilter);
	}

	bool physics::px_physics_engine::checkSphere(const vec3& center, const float radius, bool hitTriggers, uint32 layerMask)
	{
		PX_SCENE_QUERY_SETUP_CHECK();
		const PxTransform pose(createPxVec3(center - vec3(0.0f)));
		const PxSphereGeometry geometry(radius);

		return scene->overlap(geometry, pose, buffer, filterData, &queryFilter);
	}

	bool physics::px_physics_engine::checkCapsule(const vec3& center, const float radius, const float halfHeight, const quat& rotation, bool hitTriggers, uint32 layerMask)
	{
		PX_SCENE_QUERY_SETUP_CHECK();
		const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
		const PxCapsuleGeometry geometry(radius, halfHeight);
		return scene->overlap(geometry, pose, buffer, filterData, &queryFilter);
	}

	physics::px_overlap_info physics::px_physics_engine::overlapCapsule(const vec3& center, const float radius, const float halfHeight, const quat& rotation, bool hitTriggers, uint32 layerMask)
	{
		PX_SCENE_QUERY_SETUP_OVERLAP();
		std::vector<uint32_t> results;
		const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
		const PxCapsuleGeometry geometry(radius, halfHeight);
		if (!scene->overlap(geometry, pose, buffer, filterData, &queryFilter))
			return px_overlap_info(false, results);

		PX_SCENE_QUERY_COLLECT_OVERLAP();

		return px_overlap_info(true, results);
	}

	physics::px_overlap_info physics::px_physics_engine::overlapBox(const vec3& center, const vec3& halfExtents, const quat& rotation, bool hitTriggers, uint32 layerMask)
	{
		PX_SCENE_QUERY_SETUP_OVERLAP();
		std::vector<uint32_t> results;
		const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
		const PxBoxGeometry geometry(createPxVec3(halfExtents));

		if (!scene->overlap(geometry, pose, buffer, filterData, &queryFilter))
			return px_overlap_info(false, results);

		PX_SCENE_QUERY_COLLECT_OVERLAP();

		return px_overlap_info(true, results);
	}

	physics::px_overlap_info physics::px_physics_engine::overlapSphere(const vec3& center, const float radius, bool hitTriggers, uint32 layerMask)
	{
		PX_SCENE_QUERY_SETUP_OVERLAP();
		std::vector<uint32_t> results;
		const PxTransform pose(createPxVec3(center - vec3(0.0f)));
		const PxSphereGeometry geometry(radius);

		if (!scene->overlap(geometry, pose, buffer, filterData, &queryFilter))
			return px_overlap_info(false, results);

		PX_SCENE_QUERY_COLLECT_OVERLAP();

		return px_overlap_info(true, results);
	}

	void physics::px_physics_engine::stepPhysics(float stepSize)
	{
		stepper.setup(stepSize);

		static constexpr uint64 align = 16U;

		static constexpr uint64 scratchMemBlockSize = MB(32U);

		static void* scratchMemBlock = allocator.allocate(scratchMemBlockSize, align, true);

		if (!stepper.advance(scene, stepSize, scratchMemBlock, scratchMemBlockSize))
			return;

		stepper.renderDone();

		stepper.wait(scene);

#if PX_ENABLE_RAYCAST_CCD
		raycastCCD->doRaycastCCD(true);
#endif
	}

	void physics::px_physics_engine::syncTransforms()
	{
#if PX_GPU_BROAD_PHASE
		physics_lock_read lock{};
#endif

		uint32_t tempNb;
		PxActor** activeActors = scene->getActiveActors(tempNb);

		nbActiveActors.store(tempNb, std::memory_order_relaxed);

		auto gameScene = globalApp.getCurrentScene();

		for (size_t i = 0; i < nbActiveActors; ++i)
		{
			if (!activeActors[i]->userData)
				continue;

			if (auto rb = activeActors[i]->is<PxRigidDynamic>())
			{
				entity_handle* handle = static_cast<entity_handle*>(activeActors[i]->userData);
				eentity renderObject = { *handle, &gameScene->registry };

				if (!renderObject.valid())
					continue;

				const auto transform = &renderObject.getComponent<transform_component>();

				const auto& pxt = rb->getGlobalPose();
				const auto& pos = pxt.p;
				const auto& rot = pxt.q;
				transform->position = createVec3(pos);
				transform->rotation = createQuat(rot);
			}
		}

		for (size_t i = 0; i < softBodies.size(); ++i)
		{
			ref<px_soft_body> sb = softBodies[i];
			sb->copyDeformedVerticesFromGPUAsync(0);
		}
	}

	void physics::px_physics_engine::processBlastQueue()
	{
		if (unfreezeBlastQueue.size() > 0)
		{
			lock lock{ blastSync };
			auto enttScene = globalApp.getCurrentScene();

			for (auto iter = unfreezeBlastQueue.begin(); iter != unfreezeBlastQueue.end(); ++iter)
			{
				auto handle = (entity_handle)*iter;
				eentity renderEntity{ handle, &enttScene->registry };

				if (auto rb = renderEntity.getComponentIfExists<physics::px_dynamic_body_component>())
				{
					rb->setConstraints(0);
				}

			}
			unfreezeBlastQueue.clear();
		}
	}

	void physics::px_physics_engine::processSimulationEventCallbacks()
	{
		simulationEventCallback->sendCollisionEvents();
		simulationEventCallback->sendTriggerEvents();

		simulationEventCallback->clear();
	}

	ref<physics::px_soft_body> physics::px_physics_engine::addSoftBody(PxSoftBody* softBody, const PxFEMParameters& femParams, const PxTransform& transform, const PxReal density, const PxReal scale, const PxU32 iterCount)
	{
		PxVec4* simPositionInvMassPinned;
		PxVec4* simVelocityPinned;
		PxVec4* collPositionInvMassPinned;
		PxVec4* restPositionPinned;

		PxSoftBodyExt::allocateAndInitializeHostMirror(*softBody, cudaContextManager, simPositionInvMassPinned, simVelocityPinned, collPositionInvMassPinned, restPositionPinned);

		const PxReal maxInvMassRatio = 50.f;

		softBody->setParameter(femParams);
		softBody->setSolverIterationCounts(iterCount);

		PxSoftBodyExt::transform(*softBody, transform, scale, simPositionInvMassPinned, simVelocityPinned, collPositionInvMassPinned, restPositionPinned);
		PxSoftBodyExt::updateMass(*softBody, density, maxInvMassRatio, simPositionInvMassPinned);
		PxSoftBodyExt::copyToDevice(*softBody, PxSoftBodyDataFlag::eALL, simPositionInvMassPinned, simVelocityPinned, collPositionInvMassPinned, restPositionPinned);

		ref<px_soft_body> sBody = make_ref<px_soft_body>(softBody, cudaContextManager);

		softBodies.push_back(sBody);

		PX_PINNED_HOST_FREE(cudaContextManager, simPositionInvMassPinned);
		PX_PINNED_HOST_FREE(cudaContextManager, simVelocityPinned);
		PX_PINNED_HOST_FREE(cudaContextManager, collPositionInvMassPinned);
		PX_PINNED_HOST_FREE(cudaContextManager, restPositionPinned);

		return sBody;
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
		kinematicsToRemoveFlag.clear();

		newTriggerPairs.clear();
		lostTriggerPairs.clear();
	}

	void physics::px_simulation_event_callback::sendCollisionEvents()
	{
		auto enttScene = globalApp.getCurrentScene();

		if (!enttScene->registry.size())
			return;

		for (auto& c : removedCollisions)
		{
			c.thisActor->onCollisionExit(c.otherActor);
			c.swapObjects();
			LOG_MESSAGE("VISHEL");
			c.thisActor->onCollisionExit(c.otherActor);
			c.swapObjects();
			physics_holder::physicsRef->collisionExitQueue.enqueue({ c.thisActor->entityHandle, c.otherActor->entityHandle });
		}

		for (auto& c : newCollisions)
		{
			c.thisActor->onCollisionEnter(c.otherActor);
			c.swapObjects();
			LOG_MESSAGE("VOSHOL");
			c.thisActor->onCollisionEnter(c.otherActor);
			c.swapObjects();
			physics_holder::physicsRef->collisionQueue.enqueue({ c.thisActor->entityHandle, c.otherActor->entityHandle });
		}
	}

	void physics::px_simulation_event_callback::sendTriggerEvents()
	{
	}

	void physics::px_simulation_event_callback::onColliderRemoved(px_body_component* collider)
	{
		clearColliderFromCollection(collider, newTriggerPairs);
		clearColliderFromCollection(collider, lostTriggerPairs);
	}

	void physics::px_simulation_event_callback::onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)
	{
#if !_DEBUG
		PxRigidActor* act1;
		PxRigidActor* act2;
		constraints->constraint->getActors(act1, act2);

		auto rb1 = physics::physics_holder::physicsRef->actorsMap[act1];
		auto rb2 = physics::physics_holder::physicsRef->actorsMap[act1];

		if (!rb1 || !rb2)
			return;

		auto enttScene = globalApp.getCurrentScene();

		eentity entt1{ (entity_handle)rb1->entityHandle, &enttScene->registry };
		eentity entt2{ (entity_handle)rb2->entityHandle, &enttScene->registry };

		if (auto node1 = entt1.getComponentIfExists<physics::chunk_graph_manager::chunk_node>())
		{
			node1->onJointBreak();
		}

		if (auto node2 = entt2.getComponentIfExists<physics::chunk_graph_manager::chunk_node>())
		{
			node2->onJointBreak();
		}
#endif
	}

	void physics::px_simulation_event_callback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
	{
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
				//UNUSED(impulse);
				UNUSED(internalFaceIndex0);
				UNUSED(internalFaceIndex1);
			}

			collision.thisVelocity = collision.otherVelocity = PxVec3(0.0f);

			if (hasPostVelocities && iter.nextItemSet())
			{
				if (iter.contactPairIndex != i)
					continue;
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
				cookingParams.gaussMapLimit = 32;
				cookingParams.suppressTriangleMeshRemapTable = false;
				cookingParams.midphaseDesc = PxMeshMidPhase::eBVH34;
				//cookingParams.meshPreprocessParams = PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
				return PxCreateTriangleMesh(cookingParams, desc);
			}
		}
		catch (...)
		{
			LOG_ERROR("Physics> Failed to create physics triangle mesh");
		}
		return nullptr;
	}

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
				cookingParams.gaussMapLimit = 32;
				cookingParams.suppressTriangleMeshRemapTable = false;
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

physx::PxRigidDynamic* physx::createVehicleActor(const PxVehicleChassisData& chassisData, PxMaterial** wheelMaterials, PxConvexMesh** wheelConvexMeshes, const PxU32 numWheels, const PxFilterData& wheelSimFilterData, PxMaterial** chassisMaterials, PxConvexMesh** chassisConvexMeshes, const PxU32 numChassisMeshes, const PxFilterData& chassisSimFilterData, PxPhysics& physics)
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

physx::PxVehicleDriveNW* physx::instantiate4WVersion(const PxVehicleDriveNW& vehicle18W, PxPhysics& physics)
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