#include "physics/core/physics_system.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/shape_component.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"

#include <core/cpu_profiling.h>

#include <ecs/base_components/transform_component.h>
#include <ecs/update_groups.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<PhysicsSystem>("PhysicsSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))
			.method("update", &PhysicsSystem::update)(metadata("update_group", update_types::PHYSICS));
	}

	PhysicsSystem::PhysicsSystem(World* _world)
		: System(_world)
	{
	}

	void PhysicsSystem::init()
	{
		entt::registry& registry = world->get_registry();
		registry.on_construct<DynamicBodyComponent>().connect<&PhysicsSystem::on_dynamic_body_created>(this);
		registry.on_construct<StaticBodyComponent>().connect<&PhysicsSystem::on_static_body_created>(this);
	}

	void PhysicsSystem::update(float dt)
	{
		process_added_bodies();

		sync_physics_to_component_changes();

		{
			ScopedSpinLock _lock{ PhysicsHolder::physics_ref->sync };

			PhysicsHolder::physics_ref->update(dt);
		}

		sync_component_to_physics();
	}

	void PhysicsSystem::sync_physics_to_component_changes()
	{
		using namespace physx;

		for (auto [entity_handle, changed_flag, dynamic_body] : world->group(components_group<ObservableMemberChangedFlagComponent, DynamicBodyComponent>).each())
		{
			if (dynamic_body.get_rigid_actor() == nullptr)
			{
				continue;
			}

			PxRigidDynamic* body = dynamic_body.get_rigid_dynamic();

			if (body == nullptr)
			{
				continue;
			}

			if (dynamic_body.filter_group.is_changed() ||
				dynamic_body.filter_mask.is_changed())
			{
				dynamic_body.setup_filter_mask();
			}

			if (dynamic_body.use_gravity.is_changed())
			{
				body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !dynamic_body.use_gravity);
			}

			if (dynamic_body.max_angular_velocity.is_changed())
			{
				body->setMaxAngularVelocity(dynamic_body.max_angular_velocity);
			}

			if (dynamic_body.solver_position_iterations_count.is_changed() ||
				dynamic_body.solver_velocity_iterations_count.is_changed())
			{
				body->setSolverIterationCounts(dynamic_body.solver_position_iterations_count, dynamic_body.solver_velocity_iterations_count);
			}

			if (dynamic_body.sleep_threshold.is_changed())
			{
				body->setSleepThreshold(dynamic_body.sleep_threshold);
			}

			if (dynamic_body.stabilization_threshold.is_changed())
			{
				body->setStabilizationThreshold(dynamic_body.stabilization_threshold);
			}

			if (dynamic_body.max_linear_velocity.is_changed())
			{
				body->setMaxLinearVelocity(dynamic_body.max_linear_velocity);
			}

			if (dynamic_body.max_depenetration_velocity.is_changed())
			{
				body->setMaxDepenetrationVelocity(dynamic_body.max_depenetration_velocity);
			}

			if (dynamic_body.max_contact_impulse.is_changed())
			{
				body->setMaxContactImpulse(dynamic_body.max_contact_impulse);
			}

			if (dynamic_body.ccd.is_changed())
			{
				body->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, dynamic_body.ccd);
			}

			if (dynamic_body.kinematic.is_changed())
			{
				body->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, !dynamic_body.kinematic);
				body->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, !dynamic_body.kinematic);
				body->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD_FRICTION, !dynamic_body.kinematic);
				body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, dynamic_body.kinematic);
			}

			if (dynamic_body.constraints.is_changed())
			{
				body->setRigidDynamicLockFlags(static_cast<PxRigidDynamicLockFlags>(dynamic_body.constraints));
			}

			if (dynamic_body.linear_damping.is_changed())
			{
				body->setLinearDamping(dynamic_body.linear_damping);
			}

			if (dynamic_body.angular_damping.is_changed())
			{
				body->setAngularDamping(dynamic_body.angular_damping);
			}

			if (dynamic_body.angular_velocity.is_changed())
			{
				body->setAngularVelocity(create_PxVec3(dynamic_body.angular_velocity));
			}

			if (dynamic_body.simulated.is_changed())
			{
				body->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, !dynamic_body.simulated);
			}

			if (dynamic_body.linear_velocity.is_changed())
			{
				body->setLinearVelocity(create_PxVec3(dynamic_body.linear_velocity));
			}

			if (dynamic_body.mass.is_changed() || dynamic_body.center_of_mass.is_changed())
			{
				PxVec3 center_of_mass = create_PxVec3(dynamic_body.center_of_mass);

				PxRigidBodyExt::setMassAndUpdateInertia(
					*body,
					dynamic_body.mass.get(),
					&center_of_mass,
					false);

				dynamic_body.center_of_mass.get_silent_for_write() = create_vec3(body->getCMassLocalPose().p);
			}
		}

		for (auto [entity_handle, transform, static_body] : world->group(components_group<ObservableMemberChangedFlagComponent, StaticBodyComponent>).each())
		{
			if (static_body.get_rigid_actor() == nullptr)
			{
				continue;
			}

			if (static_body.filter_group.is_changed() ||
				static_body.filter_mask.is_changed())
			{
				static_body.setup_filter_mask();
			}
		}

		for (auto [entity_handle, transformm_component, dynamic_body] : world->group(components_group<TransformComponent, DynamicBodyComponent>).each())
		{
			if (dynamic_body.get_rigid_actor() == nullptr)
			{
				continue;
			}

			PxRigidDynamic* body = dynamic_body.get_rigid_dynamic();

			if (body == nullptr)
			{
				return;
			}

			for (const Force& force : std::exchange(dynamic_body.forces, {}))
			{
				if (fuzzy_equals(force.force, vec3::zero))
				{
					continue;
				}

				vec3 value = force.force;
				if (force.mode == ForceMode::IMPULSE)
				{
					const float max_contact_impulse = body->getMaxContactImpulse();
					const float value_magnitude = length(value);

					value = normalize(value) * std::min(max_contact_impulse, value_magnitude);
				}

				if (force.mode == ForceMode::FORCE || force.mode == ForceMode::IMPULSE)
				{
					PxRigidBodyExt::addForceAtLocalPos(*body,
						create_PxVec3(value),
						create_PxVec3(force.point),
						static_cast<PxForceMode::Enum>(force.mode));
				}
				else
				{
					body->addForce(create_PxVec3(force.force), static_cast<PxForceMode::Enum>(force.mode));
				}
			}

			for (const Torque& torque : std::exchange(dynamic_body.torques, {}))
			{
				if (fuzzy_equals(torque.torque, vec3::zero))
				{
					continue;
				}

				vec3 value = torque.torque;
				if (torque.mode == ForceMode::IMPULSE)
				{
					const float max_contact_impulse = body->getMaxContactImpulse();
					const float value_magnitude = length(value);

					value = normalize(value) * std::min(max_contact_impulse, value_magnitude);
				}

				body->addTorque(create_PxVec3(value),
					static_cast<PxForceMode::Enum>(torque.mode));
			}
		}
	}

	void PhysicsSystem::sync_component_to_physics()
	{
		using namespace physx;

		for (auto [entity_handle, transformm_component, dynamic_body] : world->group(components_group<TransformComponent, DynamicBodyComponent>).each())
		{
			PxRigidDynamic* body = dynamic_body.get_rigid_dynamic();

			if (body == nullptr)
			{
				return;
			}

			if (!dynamic_body.kinematic)
			{
				dynamic_body.linear_velocity.get_silent_for_write() = create_vec3(body->getLinearVelocity());
				dynamic_body.angular_velocity.get_silent_for_write() = create_vec3(body->getAngularVelocity());
			}
			else
			{
				dynamic_body.linear_velocity.get_silent_for_write() = vec3::zero;
				dynamic_body.angular_velocity.get_silent_for_write() = vec3::zero;
			}
		}
	}

	void PhysicsSystem::process_added_bodies()
	{
		using namespace physx;

		ScopedSpinLock _lock{ PhysicsHolder::physics_ref->sync };

		auto& physics_ref = PhysicsHolder::physics_ref;
		const auto& physics = physics_ref->get_physics();

		PxSceneWriteLock _scene_lock { *physics_ref->get_scene()};

		for (Entity::Handle entity_handle : std::exchange(dynamics_to_init, {}))
		{
			Entity entity = world->get_entity(entity_handle);

			DynamicBodyComponent& dynamic_body_component = entity.get_component<DynamicBodyComponent>();

			auto& colliders = physics_ref->colliders_map[entity_handle];
			if (colliders.empty())
			{
				CapsuleShapeComponent& shape_component = entity.add_component<CapsuleShapeComponent>();
				shape_component.half_height = 1.0f;
				shape_component.radius = 0.3f;
				colliders.push_back(&shape_component);
			}

			TransformComponent* transform = entity.get_component_if_exists<TransformComponent>();
			transform->type = TransformComponent::DYNAMIC;
			const vec3& pos = transform->transform.position;
			PxVec3 pospx = create_PxVec3(pos);

			const quat& q = transform->transform.rotation;
			PxQuat rotpx = create_PxQuat(q);

			void* user_data = static_cast<void*>(dynamic_body_component.component_data.get());

			dynamic_body_component.actor = PhysicsUtils::create_rigid_dynamic(PxTransform(pospx, rotpx), user_data);

			for (ShapeComponent* coll : colliders)
			{
				PxShape* shape = coll->create_shape();
				shape->userData = user_data;

				dynamic_body_component.actor->attachShape(*shape);
			}

			physics_ref->add_actor(&dynamic_body_component, dynamic_body_component.actor);

			{
				dynamic_body_component.setup_filter_mask();

				PxRigidDynamic* dynamic_body = dynamic_body_component.get_rigid_dynamic();

				dynamic_body->setLinearDamping(dynamic_body_component.linear_damping);
				dynamic_body->setAngularDamping(dynamic_body_component.angular_damping);

				if (dynamic_body_component.constraints != 0)
				{
					dynamic_body->setRigidDynamicLockFlags(static_cast<PxRigidDynamicLockFlags>(dynamic_body_component.constraints));
				}

				dynamic_body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, dynamic_body_component.kinematic);
				dynamic_body->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, dynamic_body_component.ccd);

				dynamic_body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !dynamic_body_component.use_gravity);

				dynamic_body->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, !dynamic_body_component.simulated);

				PxVec3 center_of_mass = create_PxVec3(dynamic_body_component.center_of_mass);

				PxRigidBodyExt::setMassAndUpdateInertia(
					*dynamic_body,
					dynamic_body_component.mass.get(),
					&center_of_mass,
					false);

				dynamic_body_component.center_of_mass = create_vec3(dynamic_body->getCMassLocalPose().p);
			}
		}

		for (Entity::Handle entity_handle : std::exchange(statics_to_init, {}))
		{
			Entity entity = world->get_entity(entity_handle);

			StaticBodyComponent& static_body_component = entity.get_component<StaticBodyComponent>();

			auto& colliders = physics_ref->colliders_map[entity_handle];
			if (colliders.empty())
			{
				CapsuleShapeComponent& shape_component = entity.add_component<CapsuleShapeComponent>();
				shape_component.half_height = 1.0f;
				shape_component.radius = 0.3f;
				colliders.push_back(&shape_component);
			}

			TransformComponent* transform = entity.get_component_if_exists<TransformComponent>();

			const vec3& pos = transform->transform.position;
			PxVec3 pospx = create_PxVec3(pos);

			const quat& q = transform->transform.rotation;
			PxQuat rotpx = create_PxQuat(q);

			void* user_data = static_cast<void*>(static_body_component.component_data.get());

			static_body_component.actor = PhysicsUtils::create_rigid_static(PxTransform(pospx, rotpx), user_data);

			for (ShapeComponent* coll : colliders)
			{
				PxShape* shape = coll->create_shape();
				shape->userData = user_data;

				static_body_component.actor->attachShape(*shape);
			}

			static_body_component.setup_filter_mask();

			physics_ref->add_actor(&static_body_component, static_body_component.actor);
		}

	}

	void PhysicsSystem::on_dynamic_body_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ PhysicsHolder::physics_ref->sync };

		dynamics_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}

	void PhysicsSystem::on_static_body_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ PhysicsHolder::physics_ref->sync };

		statics_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}

}
