#pragma once

#ifdef setBit
#undef setBit
#endif // setBit

#include <core/math.h>

#define NDEBUG 0

#define PX_GPU_BROAD_PHASE 0

#define PX_CONTACT_BUFFER_SIZE 128

#define PX_ENABLE_RAYCAST_CCD 0

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include "extensions/PxRaycastCCD.h"

#include <px/physics/px_rigidbody_component.h>
#include <set>
#include <unordered_map>
#include <iostream>

struct application;

#define PX_PHYSICS_ENABLED = 1

#define PX_NB_MAX_RAYCAST_HITS 64
#define PX_NB_MAX_RAYCAST_DISTANCE 128

#define PX_VEHICLE 0

using namespace physx;

#define PX_RELEASE(x)	if(x)	{ x->release(); x = nullptr;}
#define UNUSED(x) (void)(x)

struct px_raycast_info
{
	px_rigidbody_component* actor = nullptr;

	float distance = 0.0f;
	unsigned int hitCount = 0;
	vec3 position = vec3(0.0f);
};

namespace physx 
{
	static PxVec3 createPxVec3(const vec3& vec) noexcept { return PxVec3(vec.x, vec.y, vec.z); }
	static PxVec2 createPxVec2(const vec2& vec) noexcept { return PxVec2(vec.x, vec.y); }
	static PxVec3 createPxVec3(vec3&& vec) noexcept { return PxVec3(vec.x, vec.y, vec.z); }
	static PxVec2 createPxVec2(vec2&& vec) noexcept { return PxVec2(vec.x, vec.y); }

	static vec3 createVec3(const PxVec3& vec) noexcept { return vec3(vec.x, vec.y, vec.z); }
	static vec2 createVec2(const PxVec2& vec) noexcept { return vec2(vec.x, vec.y); }
	static vec3 createVec3(PxVec3&& vec) noexcept { return vec3(vec.x, vec.y, vec.z); }
	static vec2 createVec2(PxVec2&& vec) noexcept { return vec2(vec.x, vec.y); }

	static PxVec2 min(const PxVec2& a, const PxVec2& b) noexcept { return PxVec2(std::min(a.x, b.x), std::min(a.y, b.y)); }
	static PxVec3 min(const PxVec3& a, const PxVec3& b) noexcept { return PxVec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)); }

	static PxVec2 max(const PxVec2& a, const PxVec2& b) noexcept { return PxVec2(std::max(a.x, b.x), std::max(a.y, b.y)); }
	static PxVec3 max(const PxVec3& a, const PxVec3& b) noexcept { return PxVec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)); }
}

class px_snippet_gpu_load_hook : public PxGpuLoadHook
{
	virtual const char* getPhysXGpuDllName() const
	{
		return "PhysXGpu_64.dll";
	}
};

class px_query_filter : public PxQueryFilterCallback
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

	PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
	{
		return PxQueryHitType::eNONE;
	}
};

class px_simulation_filter_callback : public PxSimulationFilterCallback
{
public:
	PxFilterFlags pairFound(PxU32 pairID,
		PxFilterObjectAttributes attributes0, PxFilterData filterData0, const PxActor* a0, const PxShape* s0,
		PxFilterObjectAttributes attributes1, PxFilterData filterData1, const PxActor* a1, const PxShape* s1,
		PxPairFlags& pairFlags) override
	{
		return PxFilterFlags(PxFilterFlag::eDEFAULT);
	};

	void pairLost(PxU32 pairID,
		PxFilterObjectAttributes attributes0,
		PxFilterData filterData0,
		PxFilterObjectAttributes attributes1,
		PxFilterData filterData1,
		bool objectRemoved) override
	{

	};

	bool statusChange(PxU32& pairID, PxPairFlags& pairFlags, PxFilterFlags& filterFlags) override
	{
		return false;
	};
};

class px_collision_contact_callback : public PxSimulationEventCallback
{
public:
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override { std::cout << "onConstraintBreak\n"; }
	void onWake(physx::PxActor** actors, physx::PxU32 count) override { std::cout << "onWake\n"; }
	void onSleep(physx::PxActor** actors, physx::PxU32 count) override { std::cout << "onSleep\n"; }
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override { std::cout << "onTrigger\n"; }
	void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override { std::printf("onAdvance\n"); }
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
};

class px_CCD_contact_modification : public PxCCDContactModifyCallback
{
public:
	void onCCDContactModify(PxContactModifyPair* const pairs, PxU32 count);
};

class px_physics_engine;

struct px_physics
{
	px_physics() = default;
	~px_physics();

	void initialize();

	void release();

	void releaseScene();

	PxScene* scene = nullptr;

	PxPhysics* physics = nullptr;

	PxCooking* cooking = nullptr;

	PxPvd* pvd = nullptr;

	PxPhysicsInsertionCallback* insertationCallback = nullptr;

	PxDefaultAllocator px_allocator;

	PxCudaContextManager* cudaContextManager = nullptr;

	PxDefaultErrorCallback defaultErrorCallback;

	RaycastCCDManager* raycastCCD = nullptr;

	PxFoundation* foundation = nullptr;

	PxTolerancesScale toleranceScale;
	PxDefaultCpuDispatcher* dispatcher = nullptr;

private:
	bool released = false;

	const uint32_t nbCPUDispatcherThreads = 4;

	friend class px_physics_engine;
	friend class CollisionContactCallback;
	friend class CCDContactModification;
};

struct px_triangle_mesh
{
	PxTriangleMesh* createTriangleMesh(PxTriangleMeshDesc desc);
};

#if PX_VEHICLE

class px_wheel_filter : public PxQueryFilterCallback
{
public:
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
};

#endif

class px_physics_engine 
{
private:
	px_physics_engine(application* application) noexcept;
	~px_physics_engine();

	px_physics_engine(const px_physics_engine&) = delete;
	px_physics_engine& operator=(const px_physics_engine&) = delete;
	px_physics_engine(px_physics_engine&&) = delete;
	px_physics_engine& operator=(px_physics_engine&&) = delete;

public:
	static void initialize(application* application);

	static void release();

	void start();
	void update(float dt);

	void resetActorsVelocityAndInertia();

	void addActor(px_rigidbody_component* actor, PxRigidActor* ractor, bool addToScene);
	void removeActor(px_rigidbody_component* actor);

	static px_physics_engine* get();

	static PxPhysics* getPhysics();

	void releaseActors() noexcept;

	float frameRate = 60.0f;

	px_physics* getPhysicsAdapter() const noexcept { return physics; }

	std::set<px_rigidbody_component*> actors;
	std::unordered_map<PxRigidActor*, px_rigidbody_component*> actors_map;

	px_raycast_info raycast(px_rigidbody_component* rb, const vec3& dir, int maxDist = PX_NB_MAX_RAYCAST_DISTANCE, int maxHits = PX_NB_MAX_RAYCAST_HITS, PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eUV | PxHitFlag::eMESH_MULTIPLE);

private:
	px_physics* physics = nullptr;

	application* app = nullptr;

	bool released = false;

	static px_physics_engine* engine;

	static std::mutex sync;

	friend class px_CCD_contact_modification;
	friend class px_collision_contact_callback;
	friend struct px_rigidbody_component;
};