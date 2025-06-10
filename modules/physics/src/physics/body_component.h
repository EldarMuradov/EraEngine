// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include "core/math.h"

#include "ecs/component.h"
#include "ecs/observable_member.h"

namespace era_engine::physics
{

	const static inline uint8_t PX_FREEZE_LINEAR = (uint8_t)(
		physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y |
		physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X |
		physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z
		);

	const static inline uint8_t PX_FREEZE_ANGULAR = (uint8_t)(
		physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X |
		physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z |
		physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y
		);

	const static inline uint8_t PX_FREEZE_ALL = (uint8_t)(
		physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y |
		physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z | physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X |
		physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y | physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z
		);

	enum class ForceMode : uint8_t
	{
		NONE = 0,
		FORCE,
		IMPULSE,
		VELOCITY_CHANGE,
		ACCELERATION
	};

	class ERA_PHYSICS_API BodyComponent : public Component
	{
	public:
		BodyComponent() = default;
		BodyComponent(ref<Entity::EcsData> _data);
		virtual ~BodyComponent();

		physx::PxRigidActor* get_rigid_actor() const;

		uint32_t get_filter_mask() const;
		void set_filter_mask(uint32_t group, uint32_t mask);

		virtual void release() override;

		ERA_VIRTUAL_REFLECT(Component)

	private:
		void setup_filter_mask();
		void detach_shape(physx::PxShape* shape);

	protected:
		physx::PxRigidActor* actor = nullptr;
		physx::PxMaterial* material = nullptr;

		ObservableMember<uint32> filter_group = -1;
		ObservableMember<uint32> filter_mask = -1;

		friend class ShapeComponent;
		friend class PhysicsSystem;
		friend class PhysicsUtils;
	};

	using OnCollisionCallback = void(*)(BodyComponent*, BodyComponent*);

	class ERA_PHYSICS_API DynamicBodyComponent : public BodyComponent
	{
	public:
		DynamicBodyComponent() = default;
		DynamicBodyComponent(ref<Entity::EcsData> _data);
		virtual ~DynamicBodyComponent();

		void add_force(const vec3& force, ForceMode mode = ForceMode::FORCE);
		void add_torque(const vec3& torque, ForceMode mode = ForceMode::FORCE);

		physx::PxRigidDynamic* get_rigid_dynamic() const;

		void set_constraints(uint8_t constraints);
		uint8_t get_constraints() const;

		void set_mass_space_inertia_tensor(const vec3& tensor);

		void update_mass_and_inertia(float density);

		void enable_max_contact_impulse(bool state);

		void set_linear_velocity(const vec3& velocity);
		vec3 get_linear_velocity() const;

		void set_angular_velocity(const vec3& velocity);
		vec3 get_angular_velocity() const;

		void set_max_linear_velosity(float velocity);
		void set_max_angular_velosity(float velocity);

		vec3 get_physics_position() const;

		void set_angular_damping(float damping);
		void set_linear_damping(float damping);

		void set_threshold(float stabilization, float sleep = 0.01f);

		void manual_clear_force_and_torque();

		ObservableMember<bool> use_gravity = true;
		ObservableMember<bool> ccd = false;
		ObservableMember<bool> kinematic = false;

		ERA_VIRTUAL_REFLECT(BodyComponent)
	};

	class ERA_PHYSICS_API StaticBodyComponent : public BodyComponent
	{
	public:
		StaticBodyComponent() = default;
		StaticBodyComponent(ref<Entity::EcsData> _data);
		virtual ~StaticBodyComponent();

		physx::PxRigidStatic* get_rigid_static() const;

		ERA_VIRTUAL_REFLECT(BodyComponent)
	};
}