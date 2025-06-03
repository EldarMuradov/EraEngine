// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/shape_component.h"
#include "physics/cct_component.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"

#include "ecs/world.h"
#include "ecs/base_components/transform_component.h"

#include <rttr/registration>

namespace era_engine::physics
{

	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<BodyComponent>("BodyComponent")
			.constructor<>();

		registration::class_<DynamicBodyComponent>("DynamicBodyComponent")
			.constructor<>();

		registration::class_<StaticBodyComponent>("StaticBodyComponent")
			.constructor<>();
	}

	BodyComponent::BodyComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	BodyComponent::~BodyComponent()
	{
	}

	physx::PxRigidActor* BodyComponent::get_rigid_actor() const
	{
		return actor;
	}

	uint32_t BodyComponent::get_filter_mask() const
	{
		return filter_mask;
	}

	void BodyComponent::set_filter_mask(uint32_t group, uint32_t mask)
	{
		using namespace physx;

		filter_mask = mask;
		filter_group = group;
		auto& colliders = PhysicsHolder::physics_ref->colliders_map[component_data->entity_handle];

		for (auto& collider : colliders)
		{
			ShapeUtils::setup_filtering(collider->get_shape(), filter_group, filter_mask);
		}
	}

	void BodyComponent::release()
	{
		PhysicsHolder::physics_ref->remove_actor(this);
		PX_RELEASE(actor)
		Component::release();
	}

	void BodyComponent::detach_shape(physx::PxShape* shape)
	{
		if (actor != nullptr)
		{
			actor->detachShape(*shape);
		}
	}

	DynamicBodyComponent::DynamicBodyComponent(ref<Entity::EcsData> _data)
		: BodyComponent(_data)
	{
		using namespace physx;

		Entity entity = get_world()->get_entity(component_data->entity_handle);

		auto& physicsRef = PhysicsHolder::physics_ref;
		const auto& physics = physicsRef->get_physics();

		auto& colliders = physicsRef->colliders_map[component_data->entity_handle];
		if (colliders.empty())
		{
			entity.add_component<CapsuleShapeComponent>(1.0f, 2.0f);
			colliders.push_back(&entity.get_component<CapsuleShapeComponent>());
		}

		TransformComponent* transform = entity.get_component_if_exists<TransformComponent>();
		transform->type = TransformComponent::DYNAMIC;
		const vec3& pos = transform->transform.position;
		PxVec3 pospx = create_PxVec3(pos);

		const quat& q = transform->transform.rotation;
		PxQuat rotpx = create_PxQuat(q);

		void* user_data = static_cast<void*>(component_data.get());

		actor = PhysicsUtils::create_rigid_dynamic(PxTransform(pospx, rotpx), user_data);

		for (auto& coll : colliders)
		{
			PxShape* shape = coll->create_shape();
			shape->userData = user_data;

			actor->attachShape(*shape);
		}

		set_filter_mask(filter_group, filter_mask);

		physicsRef->add_actor(this, actor);
	}

	DynamicBodyComponent::~DynamicBodyComponent()
	{
	}

	void DynamicBodyComponent::add_force(const vec3& force, ForceMode mode)
	{
		using namespace physx;

		if (PxRigidDynamic* dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());

			if (mode == ForceMode::FORCE)
			{
				dyn->addForce(create_PxVec3(force), PxForceMode::eFORCE);
			}
			else if (mode == ForceMode::IMPULSE)
			{
				dyn->addForce(create_PxVec3(force), PxForceMode::eIMPULSE);
			}
			else if (mode == ForceMode::VELOCITY_CHANGE)
			{
				dyn->addForce(create_PxVec3(force), PxForceMode::eVELOCITY_CHANGE);
			}
			else if (mode == ForceMode::ACCELERATION)
			{
				dyn->addForce(create_PxVec3(force), PxForceMode::eACCELERATION);
			}
			else
			{
				dyn->addForce(create_PxVec3(force), PxForceMode::eFORCE);
			}
		}
	}

	void DynamicBodyComponent::add_torque(const vec3& torque, ForceMode mode)
	{
		using namespace physx;

		if (PxRigidDynamic* dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());

			if (mode == ForceMode::FORCE)
			{
				dyn->addTorque(create_PxVec3(torque), PxForceMode::eFORCE);
			}
			else if (mode == ForceMode::IMPULSE)
			{
				dyn->addTorque(create_PxVec3(torque), PxForceMode::eIMPULSE);
			}
			else if (mode == ForceMode::VELOCITY_CHANGE)
			{
				dyn->addTorque(create_PxVec3(torque), PxForceMode::eVELOCITY_CHANGE);
			}
			else if (mode == ForceMode::ACCELERATION)
			{
				dyn->addTorque(create_PxVec3(torque), PxForceMode::eACCELERATION);
			}
			else
			{
				dyn->addTorque(create_PxVec3(torque), PxForceMode::eFORCE);
			}
		}
	}

	physx::PxRigidDynamic* DynamicBodyComponent::get_rigid_dynamic() const
	{
		return actor->is<physx::PxRigidDynamic>();
	}

	void DynamicBodyComponent::set_gravity(bool use_gravity)
	{
		actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !use_gravity);
	}

	void DynamicBodyComponent::set_constraints(uint8_t constraints)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setRigidDynamicLockFlags((PxRigidDynamicLockFlags)constraints);
		}
	}

	uint8_t DynamicBodyComponent::get_constraints() const
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			return dyn->getRigidDynamicLockFlags();
		}
		return 0;
	}

	void DynamicBodyComponent::set_CCD(bool enable)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, enable);
		}
	}

	void DynamicBodyComponent::set_mass_space_inertia_tensor(const vec3& tensor)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setMassSpaceInertiaTensor(create_PxVec3(tensor));
		}
	}

	void DynamicBodyComponent::update_mass_and_inertia(float density)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			PxRigidBodyExt::updateMassAndInertia(*dyn, density);
		}
	}

	void DynamicBodyComponent::enable_max_contact_impulse(bool state)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD_MAX_CONTACT_IMPULSE, state);
		}
	}

	void DynamicBodyComponent::set_kinematic(bool kinematic)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, !kinematic);
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, !kinematic);
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD_FRICTION, !kinematic);
			dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
		}
	}

	bool DynamicBodyComponent::is_kinematic_body() const
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			return dyn->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;
		}
		return false;
	}

	void DynamicBodyComponent::set_linear_velocity(const vec3& velocity)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setLinearVelocity(create_PxVec3(velocity));
		}
	}

	vec3 DynamicBodyComponent::get_linear_velocity() const
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneReadLock lock(*PhysicsHolder::physics_ref->get_scene());
			PxVec3 vel = dyn->getLinearVelocity();
			return create_vec3(vel);
		}
		return vec3();
	}

	void DynamicBodyComponent::set_angular_velocity(const vec3& velocity)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setAngularVelocity(create_PxVec3(velocity));
		}
	}

	vec3 DynamicBodyComponent::get_angular_velocity() const
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneReadLock lock(*PhysicsHolder::physics_ref->get_scene());
			PxVec3 vel = dyn->getAngularVelocity();
			return create_vec3(vel);
		}
		return vec3();
	}

	void DynamicBodyComponent::set_max_linear_velosity(float velocity)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setMaxLinearVelocity(velocity);
		}
	}

	void DynamicBodyComponent::set_max_angular_velosity(float velocity)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setMaxAngularVelocity(velocity);
		}
	}

	vec3 DynamicBodyComponent::get_physics_position() const
	{
		using namespace physx;

		PxSceneReadLock lock(*PhysicsHolder::physics_ref->get_scene());
		PxVec3 pos = actor->getGlobalPose().p;
		return create_vec3(pos);
	}

	void DynamicBodyComponent::set_angular_damping(float damping)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setAngularDamping(damping);
		}
	}

	void DynamicBodyComponent::set_linear_damping(float damping)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setLinearDamping(damping);
		}
	}

	void DynamicBodyComponent::set_threshold(float stabilization, float sleep)
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->setStabilizationThreshold(stabilization);
			dyn->setSleepThreshold(sleep);
		}
	}

	void DynamicBodyComponent::manual_clear_force_and_torque()
	{
		using namespace physx;

		if (auto dyn = actor->is<PxRigidDynamic>())
		{
			PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
			dyn->clearForce();
			dyn->clearTorque();

			if (dyn->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
			{
				return;
			}
			dyn->setAngularVelocity(PxVec3(0.0f));
			dyn->setLinearVelocity(PxVec3(0.0f));
		}
	}

	StaticBodyComponent::StaticBodyComponent(ref<Entity::EcsData> _data)
		: BodyComponent(_data)
	{
		using namespace physx;

		Entity entity = get_world()->get_entity(component_data->entity_handle);

		auto& physicsRef = PhysicsHolder::physics_ref;
		const auto& physics = physicsRef->get_physics();

		auto& colliders = physicsRef->colliders_map[component_data->entity_handle];
		if (colliders.empty())
		{
			entity.add_component<CapsuleShapeComponent>(1.0f, 2.0f);
			colliders.push_back(&entity.get_component<CapsuleShapeComponent>());
		}

		TransformComponent* transform = entity.get_component_if_exists<TransformComponent>();

		const vec3& pos = transform->transform.position;
		PxVec3 pospx = create_PxVec3(pos);

		const quat& q = transform->transform.rotation;
		PxQuat rotpx = create_PxQuat(q);

		void* user_data = static_cast<void*>(component_data.get());

		actor = PhysicsUtils::create_rigid_static(PxTransform(pospx, rotpx), user_data);

		for (auto& coll : colliders)
		{
			PxShape* shape = coll->create_shape();
			shape->userData = user_data;

			actor->attachShape(*shape);
		}

		set_filter_mask(filter_group, filter_mask);

		physicsRef->add_actor(this, actor);
	}

	StaticBodyComponent::~StaticBodyComponent()
	{
	}

	physx::PxRigidStatic* StaticBodyComponent::get_rigid_static() const
	{
		return actor->is<physx::PxRigidStatic>();
	}

}