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
		using namespace physx;

		const auto& physics_ref = PhysicsHolder::physics_ref;
		physics_ref->update(dt);

		for (auto [entity_handle, transform, dynamic_body] : world->group(components_group<TransformComponent, DynamicBodyComponent>).each())
		{
			if (dynamic_body.filter_group.is_changed() || 
				dynamic_body.filter_mask.is_changed())
			{
				dynamic_body.setup_filter_mask();
			}

			if (dynamic_body.use_gravity.is_changed())
			{
				dynamic_body.actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !dynamic_body.use_gravity);
			}

			if (dynamic_body.ccd.is_changed())
			{
				if (auto dyn = dynamic_body.actor->is<PxRigidDynamic>())
				{
					dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, dynamic_body.ccd);
				}
			}

			if (dynamic_body.kinematic.is_changed())
			{
				if (auto dyn = dynamic_body.actor->is<PxRigidDynamic>())
				{
					PxSceneWriteLock lock(*PhysicsHolder::physics_ref->get_scene());
					dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, !dynamic_body.kinematic);
					dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_SPECULATIVE_CCD, !dynamic_body.kinematic);
					dyn->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD_FRICTION, !dynamic_body.kinematic);
					dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, dynamic_body.kinematic);
				}
			}
		}

		for (auto [entity_handle, transform, static_body] : world->group(components_group<TransformComponent, StaticBodyComponent>).each())
		{
			if (static_body.filter_group.is_changed() ||
				static_body.filter_mask.is_changed())
			{
				static_body.setup_filter_mask();
			}
		}
	}

	void PhysicsSystem::on_dynamic_body_created(entt::registry& registry, entt::entity entity_handle)
	{
		using namespace physx;

		DynamicBodyComponent& dynamic_body_component = registry.get<DynamicBodyComponent>(entity_handle);

		Entity entity = world->get_entity(entity_handle);

		auto& physics_ref = PhysicsHolder::physics_ref;
		const auto& physics = physics_ref->get_physics();

		auto& colliders = physics_ref->colliders_map[entity_handle];
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

		void* user_data = static_cast<void*>(dynamic_body_component.component_data.get());

		dynamic_body_component.actor = PhysicsUtils::create_rigid_dynamic(PxTransform(pospx, rotpx), user_data);

		for (ShapeComponent* coll : colliders)
		{
			PxShape* shape = coll->create_shape();
			shape->userData = user_data;

			dynamic_body_component.actor->attachShape(*shape);
		}

		dynamic_body_component.setup_filter_mask();

		physics_ref->add_actor(&dynamic_body_component, dynamic_body_component.actor);
	}

	void PhysicsSystem::on_static_body_created(entt::registry& registry, entt::entity entity_handle)
	{
		using namespace physx;

		StaticBodyComponent& static_body_component = registry.get<StaticBodyComponent>(entity_handle);

		Entity entity = world->get_entity(entity_handle);

		auto& physics_ref = PhysicsHolder::physics_ref;
		const auto& physics = physics_ref->get_physics();

		auto& colliders = physics_ref->colliders_map[entity_handle];
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
