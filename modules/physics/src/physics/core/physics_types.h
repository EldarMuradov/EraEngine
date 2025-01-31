#pragma once

#include "physics/physx_api.h"

#include <ecs/entity.h>

namespace era_engine::physics
{

	class PhysicsAllocatorCallback : public physx::PxAllocatorCallback
	{
	public:
		void* allocate(size_t size, const char* type_name, const char* filename, int line) override;

		void deallocate(void* ptr) override;
	};


	template<typename HitType>
	class DynamicHitBuffer : public physx::PxHitCallback<HitType>
	{
	public:
		DynamicHitBuffer() : physx::PxHitCallback<HitType>(buffer, PX_CONTACT_BUFFER_SIZE), count(0)
		{
		}

	public:
		PX_INLINE physx::PxU32 getNbAnyHits() const
		{
			return getNbTouches();
		}

		PX_INLINE const HitType& getAnyHit(const physx::PxU32 index) const
		{
			PX_ASSERT(index < getNbTouches() + physx::PxU32(this->hasBlock));
			return index < getNbTouches() ? getTouches()[index] : this->block;
		}

		PX_INLINE physx::PxU32 getNbTouches() const
		{
			return count;
		}

		PX_INLINE const HitType* getTouches() const
		{
			return buffer;
		}

		PX_INLINE const HitType& getTouch(const physx::PxU32 index) const
		{
			PX_ASSERT(index < getNbTouches());
			return buffer[index];
		}

		PX_INLINE physx::PxU32 getMaxNbTouches() const
		{
			return PX_CONTACT_BUFFER_SIZE;
		}

	protected:
		physx::PxAgain processTouches(const HitType* in_buffer, physx::PxU32 nb_hits) override
		{
			using namespace physx;
			nb_hits = min(nb_hits, PX_CONTACT_BUFFER_SIZE - count);
			for (PxU32 i = 0; i < nb_hits; i++)
			{
				buffer[count + i] = in_buffer[i];
			}
			count += nb_hits;
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
		uint32_t count;
		HitType buffer[PX_CONTACT_BUFFER_SIZE];
	};

	class OverlapCallback : public physx::PxOverlapBufferN<32>
	{
	public:
		OverlapCallback() = default;

		physx::PxAgain processTouches(const physx::PxOverlapHit* buffer, physx::PxU32 nb_hits);

		physx::PxRigidDynamic* hit_actor = nullptr;
	};

	class SimulationFilterCallback : public physx::PxSimulationFilterCallback
	{
	public:
		physx::PxFilterFlags pairFound(physx::PxU64 pairID,
			physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, const physx::PxActor* a0, const physx::PxShape* s0,
			physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, const physx::PxActor* a1, const physx::PxShape* s1,
			physx::PxPairFlags& pairFlags) override;

		void pairLost(physx::PxU64 pairID,
			physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
			physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
			bool objectRemoved) override;

		bool statusChange(physx::PxU64& pairID, physx::PxPairFlags& pairFlags, physx::PxFilterFlags& filterFlags) override;
	};

	class QueryFilter : public physx::PxQueryFilterCallback
	{
	public:
		physx::PxQueryHitType::Enum preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags) override;

		physx::PxQueryHitType::Enum postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit, const physx::PxShape* shape, const physx::PxRigidActor* actor) override;
	};

	class CharacterControllerFilterCallback : public physx::PxControllerFilterCallback
	{
	public:
		static physx::PxShape* getShape(const physx::PxController& controller);

		bool filter(const physx::PxController& a, const physx::PxController& b) override;
	};

	class ProfilerCallback : public physx::PxProfilerCallback
	{
	public:
		void* zoneStart(const char* eventName, bool detached, uint64_t contextId) override;

		void zoneEnd(void* profilerData, const char* eventName, bool detached, uint64_t contextId) override;
	};

	class ErrorReporter : public physx::PxErrorCallback
	{
	public:
		void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
	};

	class BodyComponent;

	struct RaycastInfo
	{
		BodyComponent* actor = nullptr;

		float distance = 0.0f;

		uint32_t hit_count = 0;

		vec3 position = vec3(0.0f);
	};

	struct OverlapInfo
	{
		bool is_overlapping;
		std::vector<Entity::Handle> results;
	};

	struct CollisionHandlingData
	{
		Entity::Handle id1;
		Entity::Handle id2;
	};

	struct ContactPoint
	{
		physx::PxVec3 point;

		physx::PxVec3 normal;

		float separation;
	};

	class Collision final
	{
	public:
		physx::PxVec3 get_relative_velocity() const;
		void swap_objects();

	public:
		BodyComponent* this_actor = nullptr;

		BodyComponent* other_actor = nullptr;

		physx::PxVec3 impulse;

		physx::PxVec3 this_velocity;

		physx::PxVec3 other_velocity;

		int32_t contacts_count;

		ContactPoint contacts[PX_CONTACT_BUFFER_SIZE];
	};

	class Joint;

	class SimulationEventCallback : public physx::PxSimulationEventCallback
	{
	public:
		SimulationEventCallback() = default;

		typedef std::pair<BodyComponent*, BodyComponent*> CollidersPair;

		void clear();

		void sendCollisionEvents();

		void sendTriggerEvents();

		void sendJointEvents();

		void onColliderRemoved(BodyComponent* collider);

		void onJointRemoved(Joint* joint);

		void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;
		void onWake(physx::PxActor** actors, physx::PxU32 count) override;
		void onSleep(physx::PxActor** actors, physx::PxU32 count) override;
		void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;
		void onAdvance(const physx::PxRigidBody* const* body_buffer, const physx::PxTransform* pose_buffer, const physx::PxU32 count) override;
		void onContact(const physx::PxContactPairHeader& pair_header, const physx::PxContactPair* pairs, physx::PxU32 nb_pairs) override;

	public:
		physx::PxArray<Collision> new_collisions;

		physx::PxArray<Joint*> broken_joints;

		physx::PxArray<Collision> removed_collisions;

		physx::PxArray<CollidersPair> new_trigger_pairs;

		physx::PxArray<CollidersPair> lost_trigger_pairs;
	};

}