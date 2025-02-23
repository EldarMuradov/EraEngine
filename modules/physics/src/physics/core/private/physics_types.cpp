#include "physics/core/physics_types.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/joint.h"

#include "core/memory.h"
#include "core/cpu_profiling.h"
#include "core/log.h"

namespace era_engine::physics
{

	static void clear_collider_from_collection(const BodyComponent* collider,
		physx::PxArray<SimulationEventCallback::CollidersPair>& collection)
	{
		const auto c = &collection[0];
		for (uint32_t i = 0; i < collection.size(); i++)
		{
			const SimulationEventCallback::CollidersPair& cc = c[i];
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

	void* PhysicsAllocatorCallback::allocate(size_t size, const char* type_name, const char* filename, int line)
	{
		PX_ASSERT(size < GB(1));
		return _aligned_malloc(size, 16);
	}

	void PhysicsAllocatorCallback::deallocate(void* ptr)
	{
		_aligned_free(ptr);
	}

	physx::PxAgain OverlapCallback::processTouches(const physx::PxOverlapHit* buffer, physx::PxU32 nb_hits)
	{
		for (physx::PxU32 i = 0; i < nb_hits; ++i)
		{
			physx::PxRigidDynamic* rigid_dynamic = buffer[i].actor->is<physx::PxRigidDynamic>();
			if (rigid_dynamic != nullptr)
			{
				hit_actor = rigid_dynamic;
				break;
			}
		}
		return true;
	}

	physx::PxFilterFlags SimulationFilterCallback::pairFound(physx::PxU64 pairID,
		physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, const physx::PxActor* a0, const physx::PxShape* s0,
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1, const physx::PxActor* a1, const physx::PxShape* s1,
		physx::PxPairFlags& pairFlags)
	{
		return physx::PxFilterFlags(physx::PxFilterFlag::eDEFAULT);
	};

	void SimulationFilterCallback::pairLost(physx::PxU64 pairID,
		physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
		bool objectRemoved)
	{
		std::cout << "lost\n";
	};

	bool SimulationFilterCallback::statusChange(physx::PxU64& pairID, physx::PxPairFlags& pairFlags, physx::PxFilterFlags& filterFlags)
	{
		return false;
	};

	physx::PxQueryHitType::Enum QueryFilter::preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags)
	{
		using namespace physx;

		if(shape == nullptr)
		{
			return PxQueryHitType::eNONE;
		}

		const PxFilterData shape_filter = shape->getQueryFilterData();
		if ((filterData.word0 & shape_filter.word0) == 0)
		{
			return PxQueryHitType::eNONE;
		}

		const bool hit_triggers = filterData.word2 != 0;
		if (!hit_triggers && shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
		{
			return PxQueryHitType::eNONE;
		}

		const bool block_single = filterData.word1 != 0;
		return block_single ? PxQueryHitType::eBLOCK : PxQueryHitType::eTOUCH;
	}

	physx::PxQueryHitType::Enum QueryFilter::postFilter(const physx::PxFilterData& filterData, const physx::PxQueryHit& hit, const physx::PxShape* shape, const physx::PxRigidActor* actor)
	{
		return physx::PxQueryHitType::eNONE;
	}

	physx::PxShape* CharacterControllerFilterCallback::getShape(const physx::PxController& controller)
	{
		using namespace physx;
		PxRigidDynamic* actor = controller.getActor();

		if (!actor || actor->getNbShapes() < 1)
		{
			return nullptr;
		}

		PxShape* shape = nullptr;
		actor->getShapes(&shape, 1);

		return shape;
	}

	bool CharacterControllerFilterCallback::filter(const physx::PxController& a, const physx::PxController& b)
	{
		using namespace physx;

		PxShape* shape_a = getShape(a);
		if (shape_a == nullptr)
		{
			return false;
		}

		PxShape* shape_b = getShape(b);
		if (shape_b == nullptr)
		{
			return false;
		}

		if (PxFilterObjectIsTrigger(shape_b->getFlags()))
		{
			return false;
		}

		const PxFilterData shape_filter_a = shape_a->getQueryFilterData();
		const PxFilterData shape_filter_b = shape_b->getQueryFilterData();

		return shape_filter_a.word0 & shape_filter_b.word1;
	}

	void ErrorReporter::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		if (message)
		{
			//LOG_ERROR("PhysX Error> %s %s %u %s %s %s %u", message, "Code:", static_cast<int32>(code), "Source:", file, ":", line);

			std::ostringstream stream;
			stream << message;
			std::cout << stream.str() << "\n";
		}
		else
		{
			std::cerr << "PhysX Error! \n";
		}
	}

	void* ProfilerCallback::zoneStart(const char* eventName, bool detached, uint64_t contextId)
	{
#if ENABLE_CPU_PROFILING
		recordProfileEvent(profile_event_begin_block, eventName);
#endif

		//LOG_MESSAGE("[%s] %s", eventName, "started");
		return nullptr;
	}

	void ProfilerCallback::zoneEnd(void* profilerData, const char* eventName, bool detached, uint64_t contextId)
	{
		//LOG_MESSAGE("[%s] %s", eventName, "finished");

#if ENABLE_CPU_PROFILING
		recordProfileEvent(profile_event_end_block, eventName);
#endif
	}
	
	physx::PxVec3 Collision::get_relative_velocity() const
	{
		return this_velocity - other_velocity;
	}

	void Collision::swap_objects()
	{
		if (!this_actor || !other_actor)
		{
			return;
		}
		std::swap(this_actor, other_actor);
		std::swap(this_velocity, other_velocity);
	}

	void SimulationEventCallback::clear()
	{
		new_collisions.clear();
		removed_collisions.clear();

		new_trigger_pairs.clear();
		lost_trigger_pairs.clear();

		broken_joints.clear();
	}

	void SimulationEventCallback::sendCollisionEvents()
	{
		for (auto& c : removed_collisions)
		{
			//TODO
			//c.this_actor->onCollisionExit(c.other_actor);
			//c.swap_objects();
			//c.this_actor->onCollisionExit(c.other_actor);
			//c.swap_objects();
			PhysicsHolder::physics_ref->collision_exit_queue.enqueue({
				c.this_actor->get_entity().get_handle(),
				c.other_actor->get_entity().get_handle()
				});
		}

		for (auto& c : new_collisions)
		{
			//TODO
			//c.this_actor->onCollisionEnter(c.other_actor);
			//c.swap_objects();
			//c.this_actor->onCollisionEnter(c.other_actor);
			//c.swap_objects();
			PhysicsHolder::physics_ref->collision_queue.enqueue({
				c.this_actor->get_entity().get_handle(),
				c.other_actor->get_entity().get_handle()
				});
		}
	}

	void SimulationEventCallback::sendTriggerEvents()
	{
	}

	void SimulationEventCallback::sendJointEvents()
	{
		for (auto* joint : broken_joints)
		{
			joint->state = JointComponent::BROKEN;
			if (joint->on_broken_callback)
			{
				joint->on_broken_callback(joint);
			}
		}
	}

	void SimulationEventCallback::onColliderRemoved(BodyComponent* collider)
	{
		clear_collider_from_collection(collider, new_trigger_pairs);
		clear_collider_from_collection(collider, lost_trigger_pairs);
	}

	void SimulationEventCallback::onJointRemoved(JointComponent* joint)
	{
		broken_joints.findAndReplaceWithLast(joint);
		broken_joints.popBack();
	}

	void SimulationEventCallback::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
	{
		using namespace physx;
		for (uint32_t i = 0; i < count; i++)
		{
			PxJoint* joint = reinterpret_cast<PxJoint*>(constraints[i].externalReference);
			if (joint->userData)
			{
				broken_joints.pushBack(static_cast<JointComponent*>(joint->userData));
			}
		}
	}

	void SimulationEventCallback::onWake(physx::PxActor** actors, physx::PxU32 count)
	{
	}

	void SimulationEventCallback::onSleep(physx::PxActor** actors, physx::PxU32 count)
	{
	}

	void SimulationEventCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
	{

	}

	void SimulationEventCallback::onAdvance(const physx::PxRigidBody* const* body_buffer, const physx::PxTransform* pose_buffer, const physx::PxU32 count)
	{
	}

	void SimulationEventCallback::onContact(const physx::PxContactPairHeader& pair_header, const physx::PxContactPair* pairs, physx::PxU32 nb_pairs)
	{
		using namespace physx;
		const PxU32 bufferSize = PX_CONTACT_BUFFER_SIZE;
		PxContactPairPoint contacts[bufferSize];

		Collision collision{};
		PxContactPairExtraDataIterator iter(pair_header.extraDataStream, pair_header.extraDataStreamSize);

		for (PxU32 i = 0; i < nb_pairs; i++)
		{
			const PxContactPair& cp = pairs[i];
			PxU32 nbContacts = pairs[i].extractContacts(contacts, bufferSize);

			const bool hasPostVelocities = !cp.flags.isSet(PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH);

			for (PxU32 j = 0; j < nbContacts; j++)
			{
				const PxVec3& point = contacts[j].position;
				const PxVec3& impulse = contacts[j].impulse;
				PxU32 internalFaceIndex0 = contacts[j].internalFaceIndex0;
				PxU32 internalFaceIndex1 = contacts[j].internalFaceIndex1;

				collision.impulse += impulse;

				UNUSED(point);
				UNUSED(internalFaceIndex0);
				UNUSED(internalFaceIndex1);
			}

			collision.this_velocity = collision.other_velocity = PxVec3(0.0f);

			if (hasPostVelocities && iter.nextItemSet())
			{
				if (iter.contactPairIndex != i)
				{
					continue;
				}
				if (iter.postSolverVelocity)
				{
					collision.this_velocity = iter.postSolverVelocity->linearVelocity[0];
					collision.other_velocity = iter.postSolverVelocity->linearVelocity[1];
				}
			}

			if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
			{
				auto r1 = pair_header.actors[0]->is<PxRigidActor>();
				auto r2 = pair_header.actors[1]->is<PxRigidActor>();

				if (!r1 || !r2)
				{
					return;
				}

				PxRigidActor* actor1 = r1;
				PxRigidActor* actor2 = r2;

				BodyComponent* rb1 = PhysicsHolder::physics_ref->actors_map[actor1];
				BodyComponent* rb2 = PhysicsHolder::physics_ref->actors_map[actor2];

				if (!rb1 || !rb2)
				{
					return;
				}

				collision.this_actor = rb1;
				collision.other_actor = rb2;

				if (cp.flags & PxContactPairFlag::eACTOR_PAIR_HAS_FIRST_TOUCH)
				{
					new_collisions.pushBack(collision);
				}
			}
			else if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
			{
				auto r1 = pair_header.actors[0]->is<PxRigidActor>();
				auto r2 = pair_header.actors[1]->is<PxRigidActor>();

				if (!r1 || !r2)
				{
					return;
				}

				PxRigidActor* actor1 = r1;
				PxRigidActor* actor2 = r2;

				BodyComponent* rb1 = PhysicsHolder::physics_ref->actors_map[actor1];
				BodyComponent* rb2 = PhysicsHolder::physics_ref->actors_map[actor2];

				if (!rb1 || !rb2)
				{
					return;
				}

				collision.this_actor = rb1;
				collision.other_actor = rb2;

				if (cp.flags & PxContactPairFlag::eACTOR_PAIR_LOST_TOUCH)
				{
					removed_collisions.pushBack(collision);
				}
			}
		}
	}

	physx::PxAgain ExplodeOverlapCallback::processTouches(const physx::PxOverlapHit* buffer, physx::PxU32 nbHits)
	{
		using namespace physx;
		PxSceneWriteLock lock{ *PhysicsHolder::physics_ref->get_scene()};
		for (PxU32 i = 0; i < nbHits; ++i)
		{
			PxRigidActor* actor = buffer[i].actor;
			PxRigidDynamic* rigidDynamic = actor->is<PxRigidDynamic>();
			if (rigidDynamic && !(rigidDynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
			{
				if (actorBuffer.find(rigidDynamic) == actorBuffer.end())
				{
					actorBuffer.insert(rigidDynamic);
					PxVec3 dr = rigidDynamic->getGlobalPose().transform(rigidDynamic->getCMassLocalPose()).p - worldPosition;
					float distance = dr.magnitude();
					float factor = PxClamp(1.0f - (distance * distance) / (radius * radius), 0.0f, 1.0f);
					float impulse = factor * explosiveImpulse * 1000.0f;
					PxVec3 vel = dr.getNormalized() * impulse / rigidDynamic->getMass();
					rigidDynamic->setLinearVelocity(rigidDynamic->getLinearVelocity() + vel);
				}
			}
		}
		return true;
	}

}