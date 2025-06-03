#pragma once

#include "physics_api.h"

#include "physics/physx_api.h"

#include "ecs/component.h"

namespace era_engine::physics
{
	physx::PxRevoluteJoint* create_revolute_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor);
	physx::PxDistanceJoint* create_distance_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor);
	physx::PxSphericalJoint* create_spherical_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor);
	physx::PxFixedJoint* create_fixed_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor);
	physx::PxPrismaticJoint* create_prismatic_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor);
	physx::PxD6Joint* create_d6_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor);

	class ERA_PHYSICS_API JointComponent : public Component
	{
	public:
		enum JointState : uint8_t
		{
			ENABLED,
			DISABLED,
			BROKEN
		};

		struct ERA_PHYSICS_API BaseDescriptor
		{
			weakref<Entity::EcsData> connected_entity;

			trs local_frame = trs::identity;
			trs second_local_frame = trs::identity;

			bool enable_collision = false;
		};

		JointComponent() = default;
		JointComponent(ref<Entity::EcsData> _data, const BaseDescriptor& _base_descriptor);

		virtual ~JointComponent();

		virtual void release() override;

		void set_break_force(float break_force);
		float get_break_force() const;

		physx::PxJoint* get_native_joint() const;

		std::function<void(JointComponent*)> on_broken_callback = nullptr;

		ERA_VIRTUAL_REFLECT(Component)

	protected:
		physx::PxJoint* joint = nullptr;
		BaseDescriptor base_descriptor;

	public:
		JointState state = ENABLED;
	};

	class ERA_PHYSICS_API FixedJointComponent : public JointComponent
	{
	public:
		FixedJointComponent() = default;
		FixedJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor);
		~FixedJointComponent() override;

		ERA_VIRTUAL_REFLECT(JointComponent)
	};

	class ERA_PHYSICS_API RevoluteJointComponent : public JointComponent
	{
	public:
		RevoluteJointComponent() = default;
		RevoluteJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor);
		~RevoluteJointComponent() override;

		void set_enable_drive(bool enable_drive);

		void set_drive_velocity(float drive_velocity);
		void set_drive_force_limit(float force_limit);
		void set_drive_gear_ratio(float gear_ratio);

		void set_constraint_limit(float upper, float lower);

		ERA_VIRTUAL_REFLECT(JointComponent)
	};

	class ERA_PHYSICS_API DistanceJointComponent : public JointComponent
	{
	public:
		DistanceJointComponent() = default;
		DistanceJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor);
		~DistanceJointComponent() override;

		void set_damping(float damping);
		float get_damping() const;

		void set_stiffness(float stiffness);
		float get_stiffness() const;

		void set_constraint_limit(float min_distance, float max_distance);

		ERA_VIRTUAL_REFLECT(JointComponent)
	};

	class ERA_PHYSICS_API SphericalJointComponent : public JointComponent
	{
	public:
		SphericalJointComponent() = default;
		SphericalJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor);
		~SphericalJointComponent() override;

		void set_constraint_limit(float swing_y, float swing_z);

		ERA_VIRTUAL_REFLECT(JointComponent)
	};

	class ERA_PHYSICS_API D6JointComponent : public JointComponent
	{
	public:
		D6JointComponent() = default;
		D6JointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor);
		~D6JointComponent() override;

		void set_swing_limit(float swing_y, float swing_z);
		void set_twist_limit(float twist_min, float twist_max);

		void set_linear_limit(float value, float stiffness, float damping);
		void set_distance_limit(float value, float stiffness, float damping);

		void set_motion(physx::PxD6Axis::Enum axis, physx::PxD6Motion::Enum type);

		void set_drive(physx::PxD6Drive::Enum axis, 
			float stiffness,
			float damping,
			float drive_force_limit,
			bool is_acceleration);

		void set_drive_velocity(const vec3& angular_drive_velocity, const vec3& linear_drive_velocity = vec3::zero);
		void set_drive_pose(const trs& drive_pose);

		ERA_VIRTUAL_REFLECT(JointComponent)
	};
}