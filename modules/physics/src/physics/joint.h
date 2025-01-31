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
			float break_force = std::numeric_limits<float>::max();
			bool enable_collision = true;
		};

		JointComponent() = default;
		JointComponent(ref<Entity::EcsData> _data, const BaseDescriptor& _base_descriptor);

		virtual ~JointComponent();

		virtual void release() override;

		physx::PxJoint* get_native_joint() const;

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
		virtual ~FixedJointComponent();

		ERA_VIRTUAL_REFLECT(JointComponent)
	};

	class ERA_PHYSICS_API RevoluteJointComponent : public JointComponent
	{
	public:
		struct ERA_PHYSICS_API RevoluteJointDescriptor
		{
			float upper = physx::PxPi / 4.0f;
			float lower = -physx::PxPi / 4.0f;

			float contact_distance = 0.01f;

			bool enable_drive = false;

			// Used only if enable_drive == true.
			float drive_velocity = 10.0f;
			float drive_force_limit = 100.0f;
			float drive_gear_ratio = 0.0f;
		};

		RevoluteJointComponent() = default;
		RevoluteJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor, const RevoluteJointDescriptor& _descriptor = RevoluteJointDescriptor{});
		virtual ~RevoluteJointComponent();

		ERA_VIRTUAL_REFLECT(JointComponent)
	private:
		RevoluteJointDescriptor descriptor;
	};

	class ERA_PHYSICS_API DistanceJointComponent : public JointComponent
	{
	public:
		struct ERA_PHYSICS_API DistanceJointDescriptor
		{
			bool enable_min_distance = true;
			float min_distance = 0.02f; // Used only if enable_min_distance == true.

			bool enable_max_distance = true;
			float max_distance = 10.0f; // Used only if enable_max_distance == true.

			float tolerance = 0.02f;

			bool enable_spring = false;
			// Used only if enable_spring == true.
			float damping = 0.01f;
			float stiffness = 0.01f;
		};

		DistanceJointComponent() = default;
		DistanceJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor, const DistanceJointDescriptor& _descriptor);
		virtual ~DistanceJointComponent();

		ERA_VIRTUAL_REFLECT(JointComponent)
	private:
		DistanceJointDescriptor descriptor;
	};
}