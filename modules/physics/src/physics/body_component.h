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
		FORCE = 0,
		IMPULSE,
		VELOCITY_CHANGE,
		ACCELERATION
	};

	struct ERA_PHYSICS_API Force
	{
		ForceMode mode = ForceMode::FORCE;
		vec3 force = vec3::zero;

		// Only FORCE and IMPULSE are supported, as the force required to produce a given velocity 
		// change or acceleration is underdetermined given only the desired change at a given point.
		vec3 point = vec3::zero;
	};

	struct ERA_PHYSICS_API Torque
	{
		ForceMode mode = ForceMode::FORCE;
		vec3 torque = vec3::zero;
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

		physx::PxRigidDynamic* get_rigid_dynamic() const;

		void set_mass_space_inertia_tensor(const vec3& tensor);

		void update_mass_and_inertia(float density);

		vec3 get_physics_position() const;

		ERA_VIRTUAL_REFLECT(BodyComponent)

	private:
		void manual_clear_force_and_torque();

	public:
		ObservableMember<bool> use_gravity = true;
		ObservableMember<bool> ccd = false;
		ObservableMember<bool> kinematic = false;

		ObservableMember<bool> simulated = false;

		ObservableMember<uint8_t> constraints = 0;

		ObservableMember<float> mass = 1.0f;

		ObservableMember<float> angular_damping = 0.05f;
		ObservableMember<float> linear_damping = 0.01f;

		ObservableMember<float> sleep_threshold = 0.00f;
		ObservableMember<float> stabilization_threshold = 0.00f;

		ObservableMember<uint32> solver_position_iterations_count = 4;
		ObservableMember<uint32> solver_velocity_iterations_count = 1;

		ObservableMember<float> max_angular_velocity = std::numeric_limits<float>::max();
		ObservableMember<float> max_linear_velocity = std::numeric_limits<float>::max();
		ObservableMember<float> max_contact_impulse = std::numeric_limits<float>::max();
		ObservableMember<float> max_depenetration_velocity = std::numeric_limits<float>::max();

		ObservableMember<vec3> linear_velocity = vec3::zero;
		ObservableMember<vec3> angular_velocity = vec3::zero;

		ObservableMember<vec3> center_of_mass = vec3::zero;

		std::vector<Force> forces;
		std::vector<Torque> torques;
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