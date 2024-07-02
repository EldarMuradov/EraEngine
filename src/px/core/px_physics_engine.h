// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#ifdef setBit
#undef setBit
#endif // setBit

#ifndef PX_PHYSX_STATIC_LIB
#define PX_PHYSX_STATIC_LIB
#endif

#define PX_CONTACT_BUFFER_SIZE 64

#define PVD_HOST "127.0.0.1"

#define PX_ENABLE_RAYCAST_CCD 0

#define PX_PARTICLE_USE_ALLOCATOR 1

#define PX_PHYSICS_ENABLED = 1

#define PX_NB_MAX_RAYCAST_HITS 64
#define PX_NB_MAX_RAYCAST_DISTANCE 128

#define PX_VEHICLE 1

#define PX_RELEASE(x)	if(x)	{ x->release(); x = nullptr;}
#define UNUSED(x) (void)(x)

#define PX_DEVICE_ALLOC(cudaContextManager, deviceBuffer, numElements) cudaContextManager->allocDeviceBuffer(deviceBuffer, numElements, PX_FL)
#define PX_DEVICE_ALLOC_T(T, cudaContextManager, numElements) cudaContextManager->allocDeviceBuffer<T>(numElements, PX_FL)
#define PX_DEVICE_FREE(cudaContextManager, deviceBuffer) cudaContextManager->freeDeviceBuffer(deviceBuffer);

#define PX_PINNED_HOST_ALLOC(cudaContextManager, pinnedHostBuffer, numElements) cudaContextManager->allocPinnedHostBuffer(pinnedHostBuffer, numElements, PX_FL)
#define PX_PINNED_HOST_ALLOC_T(T, cudaContextManager, numElements) cudaContextManager->allocPinnedHostBuffer<T>(numElements, PX_FL)
#define PX_PINNED_HOST_FREE(cudaContextManager, pinnedHostBuffer) cudaContextManager->freePinnedHostBuffer(pinnedHostBuffer);

#define PX_SCENE_QUERY_SETUP_SWEEP_CAST_ALL() PX_SCENE_QUERY_SETUP(true); \
		physx::PxSweepBufferN<1> buffer

#define PX_SCENE_QUERY_SETUP_SWEEP_CAST() PX_SCENE_QUERY_SETUP(false); \
		px_dynamic_hit_buffer<physx::PxSweepHit> buffer

#define PX_SCENE_QUERY_SETUP_CHECK() PX_SCENE_QUERY_SETUP(false); \
		physx::PxOverlapBufferN<1> buffer

#define PX_SCENE_QUERY_SETUP_OVERLAP() PX_SCENE_QUERY_SETUP(false); \
		px_dynamic_hit_buffer<physx::PxOverlapHit> buffer

#define PX_SCENE_QUERY_COLLECT_OVERLAP() results.clear(); \
		results.resize(buffer.getNbTouches()); \
		size_t resultSize = results.size(); \
		for (int32 i = 0; i < resultSize; i++) \
		{ \
			const auto& hit = buffer.getTouch(i); \
			results[i] = hit.shape ? *reinterpret_cast<uint32_t*>(hit.shape->userData) : 0; \
		}

#include <cuda.h>
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <extensions/PxRaycastCCD.h>
#include <extensions/PxRemeshingExt.h>
#include <extensions/PxSoftBodyExt.h>
#include <cudamanager/PxCudaContextManager.h>
#include <extensions/PxParticleExt.h>
#include <PxPBDParticleSystem.h>
#include <extensions/PxParticleClothCooker.h>
#include <cudamanager/PxCudaContext.h>

#if PX_VEHICLE
#include <vehicle/PxVehicleUtil.h>
#include <snippetutils/SnippetUtils.h>
#include <snippetvehicle2common/enginedrivetrain/EngineDrivetrain.h>
#include <snippetvehicle2common/serialization/BaseSerialization.h>
#include <snippetvehicle2common/serialization/EngineDrivetrainSerialization.h>
#include <snippetvehicle2common/SnippetVehicleHelpers.h>
#endif

#include "core/memory.h"
#include "core/cpu_profiling.h"
#include "core/math.h"
#include "core/event_queue.h"

#include "scene/components.h"

#include <concurrentqueue/concurrentqueue.h>

namespace Nv
{
	namespace Blast
	{
		class ExtImpactDamageManager;
	}
}

namespace physx
{
	using namespace era_engine;
	constexpr float PX_HALF_MAX_F32 = PX_MAX_F32 / 2.0f;

	PX_FORCE_INLINE constexpr PxU32 id(PxU32 x, PxU32 y, PxU32 numY) noexcept
	{
		return x * numY + y;
	}

	inline PxVec3 operator*(PxVec3 a, PxVec3 b) { return PxVec3(a.x * b.x, a.y * b.y, a.z * b.z); }
	inline PxVec4 operator*(PxVec4 a, PxVec4 b) { return PxVec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }

	NODISCARD PX_FORCE_INLINE PxVec4 createPxVec4(const vec3& vec) noexcept { return PxVec4(vec.x, vec.y, vec.z, 0); }
	NODISCARD PX_FORCE_INLINE PxVec4 createPxVec4(const vec4& vec) noexcept { return PxVec4(vec.x, vec.y, vec.z, vec.w); }

	NODISCARD PX_FORCE_INLINE PxVec3 createPxVec3(const vec3& vec) noexcept { return PxVec3(vec.x, vec.y, vec.z); }
	NODISCARD PX_FORCE_INLINE PxVec2 createPxVec2(const vec2& vec) noexcept { return PxVec2(vec.x, vec.y); }
	NODISCARD PX_FORCE_INLINE PxVec3 createPxVec3(vec3&& vec) noexcept { return PxVec3(vec.x, vec.y, vec.z); }
	NODISCARD PX_FORCE_INLINE PxVec2 createPxVec2(vec2&& vec) noexcept { return PxVec2(vec.x, vec.y); }

	NODISCARD PX_FORCE_INLINE PxQuat createPxQuat(const quat& q) noexcept { return PxQuat(q.x, q.y, q.z, q.w); }
	NODISCARD PX_FORCE_INLINE PxQuat createPxQuat(quat&& q) noexcept { return PxQuat(q.x, q.y, q.z, q.w); }

	NODISCARD PX_FORCE_INLINE vec3 createVec3(const PxVec3& vec) noexcept { return vec3(vec.x, vec.y, vec.z); }
	NODISCARD PX_FORCE_INLINE vec2 createVec2(const PxVec2& vec) noexcept { return vec2(vec.x, vec.y); }
	NODISCARD PX_FORCE_INLINE vec3 createVec3(PxVec3&& vec) noexcept { return vec3(vec.x, vec.y, vec.z); }
	NODISCARD PX_FORCE_INLINE vec2 createVec2(PxVec2&& vec) noexcept { return vec2(vec.x, vec.y); }

	NODISCARD PX_FORCE_INLINE quat createQuat(const PxQuat& q) noexcept { return quat(q.x, q.y, q.z, q.w); }
	NODISCARD PX_FORCE_INLINE quat createQuat(PxQuat&& q) noexcept { return quat(q.x, q.y, q.z, q.w); }
							 
	NODISCARD PX_FORCE_INLINE PxVec2 min(const PxVec2& a, const PxVec2& b) noexcept { return PxVec2(std::min(a.x, b.x), std::min(a.y, b.y)); }
	NODISCARD PX_FORCE_INLINE PxVec3 min(const PxVec3& a, const PxVec3& b) noexcept { return PxVec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)); }
							 
	NODISCARD PX_FORCE_INLINE PxVec2 max(const PxVec2& a, const PxVec2& b) noexcept { return PxVec2(std::max(a.x, b.x), std::max(a.y, b.y)); }
	NODISCARD PX_FORCE_INLINE PxVec3 max(const PxVec3& a, const PxVec3& b) noexcept { return PxVec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)); }

#if PX_VEHICLE

	enum
	{
		DRIVABLE_SURFACE = 0xffff0000,
		UNDRIVABLE_SURFACE = 0x0000ffff
	};

	enum
	{
		COLLISION_FLAG_GROUND = 1 << 0,
		COLLISION_FLAG_WHEEL = 1 << 1,
		COLLISION_FLAG_CHASSIS = 1 << 2,
		COLLISION_FLAG_OBSTACLE = 1 << 3,
		COLLISION_FLAG_DRIVABLE_OBSTACLE = 1 << 4,

		COLLISION_FLAG_GROUND_AGAINST = COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
		COLLISION_FLAG_WHEEL_AGAINST = COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE,
		COLLISION_FLAG_CHASSIS_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
		COLLISION_FLAG_OBSTACLE_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
		COLLISION_FLAG_DRIVABLE_OBSTACLE_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
	};

	enum
	{
		DRIVABLE_SURFACE_ID = 0xffffffff
	};

	enum
	{
		TIRE_TYPE_NORMAL = 0,
		TIRE_TYPE_WETS,
		TIRE_TYPE_SLICKS,
		TIRE_TYPE_ICE,
		TIRE_TYPE_MUD,
		MAX_NUM_TIRE_TYPES
	};

	inline void setupDrivableSurface(PxFilterData& filterData)
	{
		filterData.word3 = (PxU32)DRIVABLE_SURFACE;
	}

	inline void setupNonDrivableSurface(PxFilterData& filterData)
	{
		filterData.word3 = UNDRIVABLE_SURFACE;
	}

	inline void setupWheelsSimulationData(const PxF32 wheelMass, const PxF32 wheelMOI,
		const PxF32 wheelRadius, const PxF32 wheelWidth, const PxU32 numWheels,
		const PxVec3* wheelCenterActorOffsets, const PxVec3& chassisCMOffset,
		const PxF32 chassisMass, PxVehicleWheelsSimData* wheelsSimData);

	inline PxRigidDynamic* createVehicleActor
	(const PxVehicleChassisData& chassisData,
		PxMaterial** wheelMaterials, PxConvexMesh** wheelConvexMeshes, const PxU32 numWheels, const PxFilterData& wheelSimFilterData,
		PxMaterial** chassisMaterials, PxConvexMesh** chassisConvexMeshes, const PxU32 numChassisMeshes, const PxFilterData& chassisSimFilterData,
		PxPhysics& physics);

	inline PxVehicleDriveNW* instantiate4WVersion(const PxVehicleDriveNW& vehicle18W, PxPhysics& physics);

	inline void swapToLowLodVersion(const PxVehicleDriveNW& vehicle18W, PxVehicleDrive4W* vehicle4W,
		PxVehicleWheels** vehicles, PxU32 vehicleId);

	inline void swapToHighLowVersion(const PxVehicleDriveNW& vehicle4W, PxVehicleDrive4W* vehicle18W,
		PxVehicleWheels** vehicles, PxU32 vehicleId);

#endif
}

namespace era_engine
{
	struct application;
	struct eallocator;
}

namespace era_engine::physics
{
	using namespace physx;

	static inline PxVec3 gravity(0.0f, -9.81f, 0.0f);

	struct px_body_component;
	struct px_collider_component_base;
	struct px_soft_body;

	struct px_simulation_event_callback;

	struct px_physics_component_base : entity_handle_component_base
	{
		px_physics_component_base() {}
		px_physics_component_base(uint32_t handle) : entity_handle_component_base(handle){}
		virtual ~px_physics_component_base() {}
		virtual void release(bool release = false) noexcept {}
	};

	inline std::vector<PxFilterData> getFilterData(PxRigidActor* actor) noexcept
	{
		std::vector<PxShape*> shapes(actor->getNbShapes(), nullptr);
		std::vector<PxFilterData> out(shapes.size());

		actor->getShapes(&shapes[0], shapes.size());

		for (int i = 0; i < shapes.size(); ++i)
			out[i] = shapes[i]->getSimulationFilterData();

		return out;
	}

	inline bool isTrigger(const PxFilterData& data)
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

	inline void setFilterData(PxRigidActor* actor,
		const std::vector<PxFilterData>& filterData) noexcept
	{
		std::vector<PxShape*> shapes(actor->getNbShapes(), nullptr);

		actor->getShapes(&shapes[0], shapes.size());
		for (int i = 0; i < shapes.size(); ++i)
			shapes[i]->setSimulationFilterData(filterData[i]);
	}

	struct px_allocator_callback : PxAllocatorCallback
	{
		void* allocate(size_t size, const char* typeName, const char* filename, int line) override
		{
			ASSERT(size < GB(1));
			return _aligned_malloc(size, 16);
		}

		void deallocate(void* ptr) override
		{
			_aligned_free(ptr);
		}
	};

	struct collision_handling_data
	{
		uint32_t id1;
		uint32_t id2;
	};

	struct px_overlap_callback : PxOverlapBufferN<32>
	{
		px_overlap_callback() : hitActor(nullptr) {}

		PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits)
		{
			for (PxU32 i = 0; i < nbHits; ++i)
			{
				PxRigidDynamic* rigidDynamic = buffer[i].actor->is<PxRigidDynamic>();
				if (rigidDynamic)
				{
					hitActor = rigidDynamic;
					break;
				}
			}
			return true;
		}

		PxRigidDynamic* hitActor;
	};

	template<typename HitType>
	class px_dynamic_hit_buffer : public PxHitCallback<HitType>
	{
	public:
		px_dynamic_hit_buffer() : PxHitCallback<HitType>(buffer, PX_CONTACT_BUFFER_SIZE), count(0)
		{
		}

	public:
		NODISCARD PX_INLINE PxU32 getNbAnyHits() const
		{
			return getNbTouches();
		}

		NODISCARD PX_INLINE const HitType& getAnyHit(const PxU32 index) const
		{
			PX_ASSERT(index < getNbTouches() + PxU32(this->hasBlock));
			return index < getNbTouches() ? getTouches()[index] : this->block;
		}

		NODISCARD PX_INLINE PxU32 getNbTouches() const
		{
			return count;
		}

		NODISCARD PX_INLINE const HitType* getTouches() const
		{
			return buffer;
		}

		NODISCARD PX_INLINE const HitType& getTouch(const PxU32 index) const
		{
			PX_ASSERT(index < getNbTouches());
			return buffer[index];
		}

		NODISCARD PX_INLINE PxU32 getMaxNbTouches() const
		{
			return PX_CONTACT_BUFFER_SIZE;
		}

	protected:
		PxAgain processTouches(const HitType* inBuffer, PxU32 nbHits) override
		{
			nbHits = min(nbHits, PX_CONTACT_BUFFER_SIZE - count);
			for (PxU32 i = 0; i < nbHits; i++)
			{
				buffer[count + i] = inBuffer[i];
			}
			count += nbHits;
			return true;
		}

		void finalizeQuery() override
		{
			if (this->hasBlock)
			{
				processTouches(&this->block, 1);
			}
		}

	private:
		uint32 count;
		HitType buffer[PX_CONTACT_BUFFER_SIZE];
	};

#define PX_SCENE_QUERY_SETUP(blockSingle) \
const physx::PxHitFlags hitFlags = physx::PxHitFlag::ePOSITION | physx::PxHitFlag::eNORMAL | physx::PxHitFlag::eMESH_MULTIPLE | physx::PxHitFlag::eUV; \
physx::PxQueryFilterData filterData; \
filterData.flags |= physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC; \
filterData.data.word0 = layerMask; \
filterData.data.word1 = blockSingle ? 1 : 0; \
filterData.data.word2 = hitTriggers ? 1 : 0

	struct px_raycast_info
	{
		px_body_component* actor = nullptr;

		float distance = 0.0f;
		unsigned int hitCount = 0;
		vec3 position = vec3(0.0f);
	};

	struct px_overlap_info
	{
		bool isOverlapping;
		std::vector<uint32_t> results;
	};

	struct px_simulation_filter_callback : PxSimulationFilterCallback
	{
		PxFilterFlags pairFound(PxU64 pairID,
			PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor* a0, const PxShape* s0,
			PxFilterObjectAttributes attributes1, PxFilterData filterData1, const PxActor* a1, const PxShape* s1,
			PxPairFlags& pairFlags) override
		{
			return PxFilterFlags(PxFilterFlag::eDEFAULT);
		};

		void pairLost(PxU64 pairID,
			PxFilterObjectAttributes attributes0, PxFilterData filterData0,
			PxFilterObjectAttributes attributes1, PxFilterData filterData1,
			bool objectRemoved) override
		{
			std::cout << "lost\n";
		};

		bool statusChange(PxU64& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags) override
		{
			return false;
		};
	};

	struct px_snippet_gpu_load_hook : PxGpuLoadHook
	{
		virtual const char* getPhysXGpuDllName() const
		{
			return "PhysXGpu_64.dll";
		}
	};

	struct px_query_filter : public PxQueryFilterCallback
	{
		PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
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

		PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor) override
		{
			return PxQueryHitType::eNONE;
		}
	};

	class px_character_controller_filter_callback : public PxControllerFilterCallback
	{
		static PxShape* getShape(const PxController& controller)
		{
			PxRigidDynamic* actor = controller.getActor();

			if (!actor || actor->getNbShapes() < 1)
				return nullptr;

			PxShape* shape = nullptr;
			actor->getShapes(&shape, 1);

			return shape;
		}

		bool filter(const PxController& a, const PxController& b) override
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
	};

	struct px_profiler_callback : PxProfilerCallback
	{
		void* zoneStart(const char* eventName, bool detached, uint64_t contextId) override
		{
#if ENABLE_CPU_PROFILING
			recordProfileEvent(profile_event_begin_block, eventName);
#endif

			LOG_MESSAGE("[%s] %s", eventName, "started");
			return nullptr;
		}

		void zoneEnd(void* profilerData, const char* eventName, bool detached, uint64_t contextId) override
		{
			LOG_MESSAGE("[%s] %s", eventName, "finished");

#if ENABLE_CPU_PROFILING
			recordProfileEvent(profile_event_end_block, eventName);
#endif
		}
	};

	struct px_error_reporter : PxErrorCallback
	{
		void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
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
	};

	struct px_contact_point
	{
		PxVec3 Point;

		PxVec3 normal;

		float separation;
	};

	struct px_collision
	{
		PxVec3 getRelativeVelocity() const noexcept
		{
			return thisVelocity - otherVelocity;
		}

		void swapObjects() noexcept
		{
			if (!thisActor || !otherActor)
				return;
			std::swap(thisActor, otherActor);
			std::swap(thisVelocity, otherVelocity);
		}

		px_body_component* thisActor = nullptr;

		px_body_component* otherActor = nullptr;

		PxVec3 impulse;

		PxVec3 thisVelocity;

		PxVec3 otherVelocity;

		int32 contactsCount;

		px_contact_point contacts[PX_CONTACT_BUFFER_SIZE];
	};

	struct px_simulation_event_callback : PxSimulationEventCallback
	{
		px_simulation_event_callback() = default;

		typedef std::pair<px_body_component*, px_body_component*> colliders_pair;

		void clear() noexcept;

		void sendCollisionEvents() noexcept;

		void sendTriggerEvents() noexcept;

		void onColliderRemoved(px_body_component* collider) noexcept;

		void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override;
		void onWake(PxActor** actors, PxU32 count) override { std::cout << "onWake\n"; }
		void onSleep(PxActor** actors, PxU32 count) override { std::cout << "onSleep\n"; }
		void onTrigger(PxTriggerPair* pairs, PxU32 count) override { std::cout << "onTrigger\n"; }
		void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override { std::cout << "onAdvance\n"; }
		void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;

		PxArray<px_collision> newCollisions;

		PxArray<px_body_component*> kinematicsToRemoveFlag;

		PxArray<px_collision> removedCollisions;

		PxArray<colliders_pair> newTriggerPairs;

		PxArray<colliders_pair> lostTriggerPairs;
	};

	struct px_CCD_contact_modification : PxCCDContactModifyCallback
	{
		void onCCDContactModify(PxContactModifyPair* const pairs, PxU32 count);
	};

	struct px_triangle_mesh_builder
	{
		NODISCARD PxTriangleMesh* createTriangleMesh(PxTriangleMeshDesc desc);
	};

	struct px_convex_mesh_builder
	{
		NODISCARD PxConvexMesh* createConvexMesh(PxConvexMeshDesc desc);
	};

#if PX_VEHICLE

	struct px_wheel_filter : PxQueryFilterCallback
	{
		PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags) override
		{
			if (!shape)
				return PxQueryHitType::eNONE;

			const PxFilterData shapeFilter = shape->getQueryFilterData();

			if (filterData.word3 == shapeFilter.word3)
				return PxQueryHitType::eNONE;

			if ((filterData.word0 & shapeFilter.word1) && (shapeFilter.word0 & filterData.word1))
				return PxQueryHitType::eBLOCK;

			return PxQueryHitType::eNONE;
		}

		PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit, const PxShape* shape, const PxRigidActor* actor) override
		{
			return PxQueryHitType::eNONE;
		}
	};

#endif

	struct blast_fracture_event { uint32_t handle; };

	extern concurrent_event_queue<blast_fracture_event> blastFractureQueue;

	class px_physics_engine
	{
		NO_COPY(px_physics_engine)

	public:
		px_physics_engine() noexcept;
		~px_physics_engine();

		void release() noexcept;

		void start() noexcept;
		void update(float dt) noexcept;

		void startSimulation(float dt);
		void endSimulation();

		void resetActorsVelocityAndInertia() noexcept;

		void addActor(px_body_component* actor, PxRigidActor* ractor, bool addToScene);
		void removeActor(px_body_component* actor);

		ref<px_soft_body> addSoftBody(PxSoftBody* softBody, const PxFEMParameters& femParams, const PxTransform& transform, const PxReal density, const PxReal scale, const PxU32 iterCount) noexcept;

		PxPhysics* getPhysics() const noexcept { return physics; }

		PxTolerancesScale getTolerancesScale() const noexcept { return toleranceScale; }

		PxCpuDispatcher* getCPUDispatcher() const noexcept { return dispatcher; }

		PxScene* getScene() const noexcept { return scene; }

		PxCudaContextManager* getCudaContextManager() const noexcept { return cudaContextManager; }

		PxMaterial* getDefaultMaterial() const noexcept { return defaultMaterial; }

		void lockRead() noexcept { scene->lockRead(); }
		void unlockRead() noexcept { scene->unlockRead(); }
		void lockWrite() noexcept { scene->lockWrite(); }
		void unlockWrite() noexcept { scene->unlockWrite(); }

		void releaseActors() noexcept;
		void releaseScene() noexcept;

		void explode(const vec3& worldPos, float damageRadius, float explosiveImpulse) noexcept;

		// Raycasting
		px_raycast_info raycast(px_body_component* rb, const vec3& dir, int maxDist = PX_NB_MAX_RAYCAST_DISTANCE, bool hitTriggers = true, uint32_t layerMask = 0, int maxHits = PX_NB_MAX_RAYCAST_HITS);

		// Checking
		bool checkBox(const vec3& center, const vec3& halfExtents, const quat& rotation, bool hitTriggers = false, uint32 layerMask = 0)
		{
			PX_SCENE_QUERY_SETUP_CHECK();
			const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
			const PxBoxGeometry geometry(createPxVec3(halfExtents));

			return scene->overlap(geometry, pose, buffer, filterData, &queryFilter);
		}

		bool checkSphere(const vec3& center, const float radius, bool hitTriggers = false, uint32 layerMask = 0)
		{
			PX_SCENE_QUERY_SETUP_CHECK();
			const PxTransform pose(createPxVec3(center - vec3(0.0f)));
			const PxSphereGeometry geometry(radius);

			return scene->overlap(geometry, pose, buffer, filterData, &queryFilter);
		}

		bool checkCapsule(const vec3& center, const float radius, const float halfHeight, const quat& rotation, bool hitTriggers = false, uint32 layerMask = 0)
		{
			PX_SCENE_QUERY_SETUP_CHECK();
			const PxTransform pose(createPxVec3(center - vec3(0.0f)), createPxQuat(rotation));
			const PxCapsuleGeometry geometry(radius, halfHeight);
			return scene->overlap(geometry, pose, buffer, filterData, &queryFilter);
		}

		// Overlapping
		px_overlap_info overlapCapsule(const vec3& center, const float radius, const float halfHeight, const quat& rotation, bool hitTriggers = false, uint32 layerMask = 0)
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

		px_overlap_info overlapBox(const vec3& center, const vec3& halfExtents, const quat& rotation, bool hitTriggers = false, uint32 layerMask = 0)
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

		px_overlap_info overlapSphere(const vec3& center, const float radius, bool hitTriggers = false, uint32 layerMask = 0)
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

		float frameRate = 60.0f;

		std::atomic_uint32_t nbActiveActors{};

		std::set<px_body_component*> actors;

		std::unordered_map<uint32, std::vector<px_collider_component_base*>> collidersMap;

		std::vector<ref<px_soft_body>> softBodies;

		moodycamel::ConcurrentQueue<collision_handling_data> collisionQueue;
		moodycamel::ConcurrentQueue<collision_handling_data> collisionExitQueue;

		std::unordered_map<PxRigidActor*, px_body_component*> actorsMap;
		std::unordered_set<uint32_t> unfreezeBlastQueue;

		std::mutex blastSync;

		spin_lock sync;

	private:
		void stepPhysics(float stepSize) noexcept;

		void syncTransforms() noexcept;

		void processBlastQueue() noexcept;

		void processSimulationEventCallbacks() noexcept;

	private:
		PxScene* scene = nullptr;

		PxPhysics* physics = nullptr;

		PxPvd* pvd = nullptr;

		PxMaterial* defaultMaterial = nullptr;

		PxFoundation* foundation = nullptr;

		PxDefaultCpuDispatcher* dispatcher = nullptr;

		PxCudaContextManager* cudaContextManager = nullptr;

#if PX_ENABLE_RAYCAST_CCD
		RaycastCCDManager* raycastCCD = nullptr;
#endif

		px_allocator_callback allocatorCallback;
		px_error_reporter errorReporter;

		px_profiler_callback profilerCallback;

		px_simulation_filter_callback simulationFilterCallback;
		ref<px_simulation_event_callback> simulationEventCallback = nullptr;

		px_query_filter queryFilter;

		PxTolerancesScale toleranceScale;

		const uint32_t nbCPUDispatcherThreads = 4;

		eallocator allocator;

		bool released = false;
	};

	struct physics_holder
	{
		static inline ref<px_physics_engine> physicsRef = nullptr;
	};
}

#include "px/physics/px_rigidbody_component.h"
#include "px/physics/px_collider_component.h"
#include "px/physics/px_character_controller_component.h"
#include "px/features/px_particles.h"
#include "px/features/cloth/px_clothing_factory.h"
#include "px/core/px_extensions.h"