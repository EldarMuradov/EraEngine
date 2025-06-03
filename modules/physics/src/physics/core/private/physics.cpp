#include "physics/core/physics.h"
#include "physics/shape_component.h"
#include "physics/body_component.h"
#include "physics/soft_body.h"

#include "core/cpu_profiling.h"
#include "core/event_queue.h"

#include "ecs/base_components/transform_component.h"
#include "ecs/world.h"
#include "ecs/editor/editor_scene.h"

#include "omnipvd/PxOmniPvd.h"

#include "pvdruntime/OmniPvdWriter.h"
#include "pvdruntime/OmniPvdFileWriteStream.h"

namespace era_engine::physics
{

	static physx::PxOmniPvd* omni_pvd = NULL;
	const char* omni_pvd_path = "E:/OmniPVD/out.ovd";

	static physx::PxFilterFlags contact_report_filter_shader(
		physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filter_data0,
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filter_data1,
		physx::PxPairFlags& pair_flags, const void* constant_block, physx::PxU32 constant_block_size)
	{
		UNUSED(constant_block_size);
		UNUSED(constant_block);

		if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
		{
			pair_flags = physx::PxPairFlag::eTRIGGER_DEFAULT;
			return physx::PxFilterFlag::eDEFAULT;
		}

		if ((filter_data0.word0 & filter_data1.word1) && (filter_data1.word0 & filter_data0.word1))
		{
			pair_flags = physx::PxPairFlag::eCONTACT_DEFAULT;
			pair_flags |= physx::PxPairFlag::eDETECT_CCD_CONTACT;
			pair_flags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pair_flags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
			pair_flags |= physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
			pair_flags |= physx::PxPairFlag::ePOST_SOLVER_VELOCITY;
			pair_flags |= physx::PxPairFlag::eNOTIFY_CONTACT_POINTS;
			pair_flags |= physx::PxPairFlag::eSOLVE_CONTACT;
			pair_flags |= physx::PxPairFlag::eDETECT_DISCRETE_CONTACT;

			return physx::PxFilterFlag::eDEFAULT;
		}

		return physx::PxFilterFlag::eSUPPRESS;
	}

	static void clearColliderFromCollection(const physics::BodyComponent* collider,
		physx::PxArray<SimulationEventCallback::CollidersPair>& collection)
	{
		const auto c = &collection[0];
		for (uint32 i = 0; i < collection.size(); i++)
		{
			const SimulationEventCallback::CollidersPair cc = c[i];
			if (cc.first == collider || cc.second == collider)
			{
				collection.remove(i--);
				if (collection.empty())
				{
					break;
				}
			}
		}
	}

	Physics::Physics(const PhysicsDescriptor& _descriptor)
		: descriptor(_descriptor)
	{
		using namespace physx;

		allocator.initialize(MB(256U));

		allocator_callback = new PhysicsAllocatorCallback();
		foundation = PxCreateFoundation(PX_PHYSICS_VERSION, *allocator_callback, error_reporter);

		if (!foundation)
		{
			throw std::exception("Failed to create {PxFoundation}. Error in {PhysicsEngine} ctor.");
		}

		if (descriptor.broad_phase == PxBroadPhaseType::eGPU && PxGetSuggestedCudaDeviceOrdinal(foundation->getErrorCallback()) < 0)
		{
			throw std::exception("Failed to create {PxFoundation}. Error in {PhysicsEngine} ctor.");
		}

		pvd = PxCreatePvd(*foundation);

		if (descriptor.enable_pvd)
		{
			omni_pvd = PxCreateOmniPvd(*foundation);
			if (!omni_pvd)
			{
				printf("Error: could not create PxOmniPvd!");
				return;
			}
			OmniPvdWriter* omniWriter = omni_pvd->getWriter();
			if (!omniWriter)
			{
				printf("Error: could not get an instance of PxOmniPvdWriter!");
				return;
			}
			OmniPvdFileWriteStream* fStream = omni_pvd->getFileWriteStream();
			if (!fStream)
			{
				printf("Error: could not get an instance of PxOmniPvdFileWriteStream!");
				return;
			}
			fStream->setFileName(omni_pvd_path);
			omniWriter->setWriteStream(static_cast<OmniPvdWriteStream&>(*fStream));
		}

		tolerance_scale.length = 1.0f;
		tolerance_scale.speed = 9.81f;
		physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, tolerance_scale, true, pvd, omni_pvd);

		if (!PxInitExtensions(*physics, pvd))
		{
			LOG_ERROR("Physics> Failed to initialize extensions.");
		}

		default_material = physics->createMaterial(0.7f, 0.7f, 0.8f);

		if (descriptor.enable_pvd && physics->getOmniPvd())
		{
			ASSERT(omni_pvd->startSampling());
		}

		dispatcher = PxDefaultCpuDispatcherCreate(nb_cpu_dispatcher_threads);

		PxSceneDesc sceneDesc(tolerance_scale);
		sceneDesc.gravity = gravity;
		sceneDesc.cpuDispatcher = dispatcher;
		sceneDesc.solverType = PxSolverType::eTGS;
		sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
		sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
		sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
		sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

		if (descriptor.broad_phase == PxBroadPhaseType::eGPU)
		{
			PxCudaContextManagerDesc cuda_context_manager_desc;
			cuda_context_manager = PxCreateCudaContextManager(*foundation, cuda_context_manager_desc, &profiler_callback);
			sceneDesc.cudaContextManager = cuda_context_manager;
			sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
			sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
			sceneDesc.gpuMaxNumPartitions = 8;
		}

		sceneDesc.broadPhaseType = descriptor.broad_phase;

		sceneDesc.filterShader = contact_report_filter_shader;

		simulation_event_callback = make_ref<SimulationEventCallback>();

		sceneDesc.simulationEventCallback = simulation_event_callback.get();

		scene = physics->createScene(sceneDesc);

		scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
		scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 1.0f);
		scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_FNORMALS, 1.0f);
		scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);

		stepper = new FixedStepper();

#if PX_ENABLE_RAYCAST_CCD
		raycastCCD = new RaycastCCDManager(scene);
#endif

#if PX_VEHICLE
		if (!PxInitVehicleSDK(*physics))
		{
			//LOG_ERROR("Physics> Failed to initialize PxVehicleSDK.");
		}

		PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, 1));
		PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
#endif
	}

	Physics::~Physics()
	{
		release();
	}

	physx::PxScene* Physics::get_scene() const
	{
		return scene;
	}

	physx::PxPhysics* Physics::get_physics() const
	{
		return physics;
	}

	physx::PxMaterial* Physics::get_default_material() const
	{
		return default_material;
	}

	physx::PxCudaContextManager* Physics::get_cuda_context_manager() const
	{
		return cuda_context_manager;
	}

	physx::PxCpuDispatcher* Physics::get_cpu_dispatcher() const
	{
		return dispatcher;
	}

	physx::PxTolerancesScale Physics::get_tolerance_scale() const
	{
		return tolerance_scale;
	}

	bool Physics::is_gpu() const
	{
		return descriptor.broad_phase == physx::PxBroadPhaseType::eGPU;
	}

	void Physics::release()
	{
		ScopedSpinLock lock{ sync };

#if PX_VEHICLE
		physx::PxCloseVehicleSDK();
#endif

		PxCloseExtensions();

		if (scene)
		{
			scene->flushSimulation();
		}

		{
			PX_RELEASE(default_material)
			PX_RELEASE(scene)
			delete stepper;
			PX_RELEASE(physics)
			PX_RELEASE(dispatcher)
			PX_RELEASE(pvd)
			PX_RELEASE(omni_pvd)
			PX_RELEASE(cuda_context_manager)
			PX_RELEASE(foundation)
		}

		delete allocator_callback;

#if PX_ENABLE_RAYCAST_CCD
		delete raycast_ccd;
		raycast_ccd = nullptr;
#endif
	}

	void Physics::start()
	{
	}

	void Physics::update(float dt)
	{
		start_simulation(dt);
		end_simulation(dt);
	}

	void Physics::start_simulation(float dt)
	{
		using namespace physx;
		recordProfileEvent(profile_event_begin_block, "PhysX update");

		const float stepSize = 1.0f / frame_rate;

		static constexpr uint64 align = 16U;

		static constexpr uint64 scratchMemBlockSize = MB(32U);

		static void* scratchMemBlock = allocator.allocate(scratchMemBlockSize, align, true);

		if (is_gpu() || !use_stepper)
		{
			PxSceneWriteLock lock{ *scene };
			scene->simulate(stepSize, NULL, scratchMemBlock, scratchMemBlockSize);
			scene->fetchResults(true);
		}
		else
		{
			stepper->setup(stepSize);

			if (!stepper->advance(scene, stepSize, scratchMemBlock, scratchMemBlockSize))
			{
				return;
			}

			stepper->renderDone();
		}
	}

	void Physics::end_simulation(float dt)
	{
		if (!is_gpu() && use_stepper)
		{
			stepper->wait(scene);
		}

		{
			CPU_PROFILE_BLOCK("PhysX process simulation event callbacks steps");
			process_simulation_event_callbacks();
		}

		{
			CPU_PROFILE_BLOCK("PhysX sync transforms steps");
			sync_transforms();
		}

		recordProfileEvent(profile_event_end_block, "PhysX update");
	}

	void Physics::reset_actors_velocity_and_inertia()
	{
		using namespace physx;

		PxSceneWriteLock lock{ *scene };
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

	void Physics::add_shape_to_entity_data(ShapeComponent* shape)
	{
		ScopedSpinLock lock{ sync };
		colliders_map[shape->get_handle()].push_back(shape);
	}

	void Physics::remove_shape_from_entity_data(ShapeComponent* shape)
	{
		ScopedSpinLock lock{ sync };

		auto& shapes = colliders_map[shape->get_handle()];
		const auto& end = shapes.end();
		for (auto iter = shapes.begin(); iter != end; ++iter)
		{
			if (*iter == shape)
			{
				shapes.erase(iter);
				break;
			}
		}
	}

	void Physics::add_actor(BodyComponent* actor, physx::PxRigidActor* physx_actor)
	{
		using namespace physx;

		{
			PxSceneWriteLock lock{ *scene };

			scene->addActor(*physx_actor);
		}

#if PX_ENABLE_RAYCAST_CCD
		if (auto r = ractor->is<PxRigidDynamic>())
		{
			PxShape* shape = nullptr;

			r->getShapes(&shape, 1);

			raycastCCD->registerRaycastCCDObject(r, shape);
		}
#endif

		{
			ScopedSpinLock l{ sync };
			actors.emplace(actor);
			actors_map.insert(std::make_pair(physx_actor, actor));
		}
	}

	void Physics::remove_actor(BodyComponent* actor)
	{
		using namespace physx;

		{
			ScopedSpinLock l{ sync };
			actors.erase(actor);
			actors_map.erase(actor->get_rigid_actor());
		}

		{
			PxSceneWriteLock lock{ *scene };

			scene->removeActor(*actor->get_rigid_actor());
		}
	}

	void Physics::release_scene()
	{
		using namespace physx;
		ScopedSpinLock l{ sync };

		PxU32 size;
		auto actors = scene->getActiveActors(size);

		PxSceneWriteLock lock{ *scene };
		scene->removeActors(actors, size);
	}

	void Physics::explode(const vec3& world_pos, float damage_radius, float explosive_impulse)
	{
		using namespace physx;
		PxVec3 pos = create_PxVec3(world_pos);
		ExplodeOverlapCallback overlap_callback(pos, damage_radius, explosive_impulse);
		scene->overlap(PxSphereGeometry(damage_radius), PxTransform(pos), overlap_callback);
	}

	RaycastInfo Physics::raycast(const BodyComponent* rb, const vec3& dir, int max_dist, bool hit_triggers, uint32_t layer_mask, int max_hits)
	{
		using namespace physx;

		const auto& pose = rb->get_rigid_actor()->getGlobalPose().p - PxVec3(0.0f, 1.5f, 0.0f);

		PX_SCENE_QUERY_SETUP(true);
		PxRaycastBuffer buffer;
		PxVec3 d = create_PxVec3(dir);
		bool status = scene->raycast(pose, d, max_dist, buffer, hitFlags, PxQueryFilterData(), &queryFilter);

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
			{
				return RaycastInfo{
						actor,
						hitInfo1.distance,
						buffer.getNbAnyHits(),
						vec3(hitInfo1.position.x, hitInfo1.position.y, hitInfo1.position.z)
					};
			}
			
			else if (buffer.getNbAnyHits() > 1)
			{
				const auto& hitInfo2 = buffer.getAnyHit(1);

				actor = actors_map[hitInfo2.actor];

				return RaycastInfo {
						actor,
						hitInfo2.distance,
						buffer.getNbAnyHits(),
						vec3(hitInfo2.position.x, hitInfo2.position.y, hitInfo2.position.z)
					};
			}
		}

		return RaycastInfo();
	}

	bool Physics::check_box(const vec3& center, const vec3& half_extents, const quat& rotation, bool hit_triggers, uint32 layer_mask)
	{
		using namespace physx;

		PX_SCENE_QUERY_SETUP_CHECK();
		const PxTransform pose(create_PxVec3(center - vec3(0.0f)), create_PxQuat(rotation));
		const PxBoxGeometry geometry(create_PxVec3(half_extents));

		return scene->overlap(geometry, pose, buffer, filterData, &queryFilter);
	}

	bool Physics::check_sphere(const vec3& center, const float radius, bool hit_triggers, uint32 layer_mask)
	{
		using namespace physx;

		PX_SCENE_QUERY_SETUP_CHECK();
		const PxTransform pose(create_PxVec3(center - vec3(0.0f)));
		const PxSphereGeometry geometry(radius);

		return scene->overlap(geometry, pose, buffer, filterData, &queryFilter);
	}

	bool Physics::check_capsule(const vec3& center, const float radius, const float half_height, const quat& rotation, bool hit_triggers, uint32 layer_mask)
	{
		using namespace physx;

		PX_SCENE_QUERY_SETUP_CHECK();
		const PxTransform pose(create_PxVec3(center - vec3(0.0f)), create_PxQuat(rotation));
		const PxCapsuleGeometry geometry(radius, half_height);
		return scene->overlap(geometry, pose, buffer, filterData, &queryFilter);
	}

	OverlapInfo Physics::overlap_capsule(const vec3& center, const float radius, const float half_height, const quat& rotation, bool hit_triggers, uint32 layer_mask)
	{
		using namespace physx;

		PX_SCENE_QUERY_SETUP_OVERLAP();
		std::vector<Entity::Handle> results;
		const PxTransform pose(create_PxVec3(center - vec3(0.0f)), create_PxQuat(rotation));
		const PxCapsuleGeometry geometry(radius, half_height);
		if (!scene->overlap(geometry, pose, buffer, filterData, &queryFilter))
		{
			return OverlapInfo(false, results);
		}

		PX_SCENE_QUERY_COLLECT_OVERLAP();

		return OverlapInfo(true, results);
	}

	OverlapInfo Physics::overlap_box(const vec3& center, const vec3& half_extents, const quat& rotation, bool hit_triggers, uint32 layer_mask)
	{
		using namespace physx;
		PX_SCENE_QUERY_SETUP_OVERLAP();
		std::vector<Entity::Handle> results;
		const PxTransform pose(create_PxVec3(center - vec3(0.0f)), create_PxQuat(rotation));
		const PxBoxGeometry geometry(create_PxVec3(half_extents));

		if (!scene->overlap(geometry, pose, buffer, filterData, &queryFilter))
		{
			return OverlapInfo(false, results);
		}

		PX_SCENE_QUERY_COLLECT_OVERLAP();

		return OverlapInfo(true, results);
	}

	OverlapInfo Physics::overlap_sphere(const vec3& center, const float radius, bool hit_triggers, uint32 layer_mask)
	{
		using namespace physx;
		PX_SCENE_QUERY_SETUP_OVERLAP();
		std::vector<Entity::Handle> results;
		const PxTransform pose(create_PxVec3(center - vec3(0.0f)));
		const PxSphereGeometry geometry(radius);

		if (!scene->overlap(geometry, pose, buffer, filterData, &queryFilter))
		{
			return OverlapInfo(false, results);
		}

		PX_SCENE_QUERY_COLLECT_OVERLAP();

		return OverlapInfo(true, results);
	}

	void Physics::step_physics(float step_size)
	{
		using namespace physx;
		stepper->setup(step_size);

		static constexpr uint64 align = 16U;

		static constexpr uint64 scratchMemBlockSize = MB(32U);

		static void* scratchMemBlock = allocator.allocate(scratchMemBlockSize, align, true);

		if (!stepper->advance(scene, step_size, scratchMemBlock, scratchMemBlockSize))
		{
			return;
		}

		stepper->renderDone();

		stepper->wait(scene);

#if PX_ENABLE_RAYCAST_CCD
		raycastCCD->doRaycastCCD(true);
#endif
	}

	void Physics::sync_transforms()
	{
		using namespace physx;

		PxSceneReadLock lock{ *scene };

		uint32_t tempNb;
		PxActor** activeActors = scene->getActiveActors(tempNb);
		nb_active_actors.store(tempNb, std::memory_order_relaxed);

		for (size_t i = 0; i < nb_active_actors; ++i)
		{
			if (!activeActors[i]->userData)
			{
				continue;
			}

			if (auto rb = activeActors[i]->is<PxRigidDynamic>())
			{
				Entity::EcsData* data = static_cast<Entity::EcsData*>(activeActors[i]->userData);

				if (data == nullptr)
				{
					continue;
				}

				// Check joint shape sync.
				{
					auto& colliders = colliders_map[data->entity_handle];

					if (!colliders.empty())
					{
						auto& shape_component = colliders.front(); // Only for single-shape entities.

						if (shape_component->attachment_active)
						{
							continue;
						}
					}
				}

				TransformComponent& transform =	data->native_registry->get<TransformComponent>(data->entity_handle);

				const auto& pxt = rb->getGlobalPose();
				const auto& pos = pxt.p;
				const auto& rot = pxt.q;
				transform.transform.position = create_vec3(pos);
				transform.transform.rotation = create_quat(rot);
			}
		}

		for (auto&& [name, world] : get_worlds())
		{
			for (auto [entity_handle, transform, soft_body] : world->group(components_group<TransformComponent, SoftBodyComponent>).each())
			{
				soft_body.copy_deformed_vertices_from_gpu_async(0);
			}
		}
	}

	void Physics::process_simulation_event_callbacks()
	{
		simulation_event_callback->sendCollisionEvents();
		simulation_event_callback->sendTriggerEvents();
		simulation_event_callback->sendJointEvents();

		simulation_event_callback->clear();
	}

}