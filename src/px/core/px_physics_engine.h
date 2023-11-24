#pragma once

#ifdef setBit
#undef setBit
#endif // setBit

#include <core/math.h>

#define NDEBUG 0

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>

#include <px/physics/px_rigidbody_component.h>
#include <set>
#include <unordered_map>
#include <iostream>

struct application;

#define PX_PHYSICS_ENABLED = 1

using namespace physx;

#define PX_RELEASE(x)	if(x)	{ x->release(); x = nullptr;}
#define UNUSED(x) (void)(x)

struct px_raycast_info
{
	px_rigidbody_component* actor;

	float distance;
	unsigned int hitCount;
	vec3 position;
};

namespace physx 
{
	static PxVec2 min(PxVec2 a, PxVec2 b) noexcept { return PxVec2(std::min(a.x, b.x), std::min(a.y, b.y)); }
	static PxVec3 min(PxVec3 a, PxVec3 b) noexcept { return PxVec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)); }

	static PxVec2 max(PxVec2 a, PxVec2 b) noexcept { return PxVec2(std::max(a.x, b.x), std::max(a.y, b.y)); }
	static PxVec3 max(PxVec3 a, PxVec3 b) noexcept { return PxVec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)); }
}

class SnippetGpuLoadHook : public PxGpuLoadHook
{
	virtual const char* getPhysXGpuDllName() const
	{
		return "PhysXGpu_64.dll";
	}
};

class ESGS_SimulationFilterCallback : public PxSimulationFilterCallback
{
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

class ESGS_CollisionContactCallback : public PxSimulationEventCallback
{
public:
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override { std::cout << "onConstraintBreak\n"; }
	void onWake(physx::PxActor** actors, physx::PxU32 count) override { std::cout << "onWake\n"; }
	void onSleep(physx::PxActor** actors, physx::PxU32 count) override { std::cout << "onSleep\n"; }
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override { std::cout << "onTrigger\n"; }
	void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override { std::printf("onAdvance\n"); }
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
};

class ESGS_CCDContactModification : public PxCCDContactModifyCallback
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

	PxDefaultErrorCallback defaultErrorCallback;

	PxFoundation* foundation = nullptr;

	PxTolerancesScale toleranceScale;
	PxDefaultCpuDispatcher* dispatcher = NULL;

private:
	bool released = false;

	friend class px_physics_engine;
	friend class ESGS_CollisionContactCallback;
	friend class ESGS_CCDContactModification;
};

struct px_triangle_mesh
{
	PxTriangleMesh* createTriangleMesh(PxTriangleMeshDesc desc);
};

class px_physics_engine 
{
public:
	px_physics_engine(application* application) noexcept;
	~px_physics_engine();

	static void initialize(application* application);

	static void release();

	void start();
	void update(float dt);

	void addActor(px_rigidbody_component* actor, PxRigidActor* ractor);
	void removeActor(px_rigidbody_component* actor);

	static px_physics_engine* get();

	static PxPhysics* getPhysics();

	void releaseActors() noexcept;

	float frameRate = 120.0f;

	px_physics* getPhysicsAdapter() const noexcept { return physics; }

	std::set<px_rigidbody_component*> actors;

	px_raycast_info raycast(px_rigidbody_component* rb, const vec3& origin, const vec3& dir, int maxDist = 100, int maxHits = 1, PxHitFlags hitFlags = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL | PxHitFlag::eUV);

private:
	px_physics* physics = nullptr;

	application* app = nullptr;

	bool released = false;

	std::unordered_map<PxRigidActor*, px_rigidbody_component*> actors_map;

	static px_physics_engine* engine;

	static std::mutex sync;

	friend class ESGS_CCDContactModification;
	friend class ESGS_CollisionContactCallback;
	friend class px_rigidbody_component;
};