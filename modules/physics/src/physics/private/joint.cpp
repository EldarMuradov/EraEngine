#include "physics/joint.h"
#include "physics/core/physics.h"
#include "physics/core/physics_utils.h"
#include "physics/body_component.h"

#include <rttr/registration>

namespace era_engine::physics
{

	physx::PxRevoluteJoint* create_revolute_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxRevoluteJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxDistanceJoint* create_distance_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxDistanceJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxSphericalJoint* create_spherical_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxSphericalJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxFixedJoint* create_fixed_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxFixedJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxPrismaticJoint* create_prismatic_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxPrismaticJointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	physx::PxD6Joint* create_d6_joint(physx::PxRigidActor* first_actor, physx::PxRigidActor* second_actor)
	{
		auto* physics = PhysicsHolder::physics_ref->get_physics();

		return physx::PxD6JointCreate(*physics,
			first_actor, first_actor->getGlobalPose(),
			second_actor, second_actor->getGlobalPose());
	}

	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::enumeration<JointComponent::State>("State")
		(
			value("ENABLED", JointComponent::State::ENABLED),
			value("DISABLED", JointComponent::State::DISABLED),
			value("BROKEN", JointComponent::State::BROKEN)
		);

		registration::class_<JointComponent>("JointComponent")
			.constructor<>();

		registration::class_<FixedJointComponent>("FixedJointComponent")
			.constructor<>();

		registration::class_<RevoluteJointComponent>("RevoluteJointComponent")
			.constructor<>();

		registration::class_<DistanceJointComponent>("DistanceJointComponent")
			.constructor<>();

		registration::class_<SphericalJointComponent>("SphericalJointComponent")
			.constructor<>();

		registration::class_<D6JointComponent>("D6JointComponent")
			.constructor<>();
	}

	JointComponent::JointComponent(ref<Entity::EcsData> _data, const BaseDescriptor& _base_descriptor)
		: Component(_data), base_descriptor(_base_descriptor)
	{
		enable_collision.set_component(ComponentPtr(this));
		break_force.set_component(ComponentPtr(this));
	}

	JointComponent::~JointComponent()
	{
	}

	void JointComponent::release()
	{
		PX_RELEASE(joint)
	}

	JointComponent::State JointComponent::get_state() const
	{
		return state;
	}

	physx::PxJoint* JointComponent::get_native_joint() const
	{
		return joint;
	}

	EntityPtr JointComponent::get_first_entity_ptr() const
	{
		return base_descriptor.connected_entity;
	}

	EntityPtr era_engine::physics::JointComponent::get_second_entity_ptr() const
	{
		return base_descriptor.second_connected_entity;
	}

	FixedJointComponent::FixedJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
		: JointComponent(_data, _base_descriptor)
	{
		using namespace physx;

		PxFixedJoint* created_joint = create_fixed_joint(PhysicsUtils::get_body_component(base_descriptor.connected_entity.get_data())->get_rigid_actor(),
								   PhysicsUtils::get_body_component(base_descriptor.second_connected_entity.get_data())->get_rigid_actor());
		created_joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, enable_collision);

		created_joint->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		created_joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

		joint = created_joint;
		joint->userData = this;
	}

	FixedJointComponent::~FixedJointComponent()
	{
	}

	RevoluteJointComponent::RevoluteJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
		: JointComponent(_data, _base_descriptor)
	{
		using namespace physx;

		auto* physics = PhysicsHolder::physics_ref->get_physics();

		PxRevoluteJoint* created_joint = physx::PxRevoluteJointCreate(*physics,
			PhysicsUtils::get_body_component(base_descriptor.connected_entity.get_data())->get_rigid_actor(), create_PxTransform(_base_descriptor.local_frame),
			PhysicsUtils::get_body_component(base_descriptor.second_connected_entity.get_data())->get_rigid_actor(), create_PxTransform(_base_descriptor.second_local_frame));

		created_joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, enable_collision);

		created_joint->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		created_joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

		joint = created_joint;
		joint->userData = this;
	}

	RevoluteJointComponent::~RevoluteJointComponent()
	{
	}

	void RevoluteJointComponent::set_enable_drive(bool enable_drive)
	{
		using namespace physx;
		if(PxRevoluteJoint* revolute_joint = joint->is<PxRevoluteJoint>())
		{
			revolute_joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, enable_drive);
		}
	}

	void RevoluteJointComponent::set_drive_velocity(float drive_velocity)
	{
		using namespace physx;
		if (PxRevoluteJoint* revolute_joint = joint->is<PxRevoluteJoint>())
		{
			revolute_joint->setDriveVelocity(drive_velocity);
		}
	}

	void RevoluteJointComponent::set_drive_force_limit(float force_limit)
	{
		using namespace physx;
		if (PxRevoluteJoint* revolute_joint = joint->is<PxRevoluteJoint>())
		{
			revolute_joint->setDriveForceLimit(force_limit);
		}
	}

	void RevoluteJointComponent::set_drive_gear_ratio(float gear_ratio)
	{
		using namespace physx;
		if (PxRevoluteJoint* revolute_joint = joint->is<PxRevoluteJoint>())
		{
			revolute_joint->setDriveGearRatio(gear_ratio);
		}
	}

	void RevoluteJointComponent::set_constraint_limit(float upper, float lower)
	{
		using namespace physx;
		if (PxRevoluteJoint* revolute_joint = joint->is<PxRevoluteJoint>())
		{
			revolute_joint->setRevoluteJointFlag(PxRevoluteJointFlag::eLIMIT_ENABLED, true);
			revolute_joint->setLimit(PxJointAngularLimitPair{ lower, upper });
		}
	}

	DistanceJointComponent::DistanceJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
		: JointComponent(_data, _base_descriptor)
	{
		using namespace physx;

		auto* physics = PhysicsHolder::physics_ref->get_physics();

		PxDistanceJoint* created_joint = physx::PxDistanceJointCreate(*physics,
			PhysicsUtils::get_body_component(base_descriptor.connected_entity.get_data())->get_rigid_actor(), create_PxTransform(_base_descriptor.local_frame),
			PhysicsUtils::get_body_component(base_descriptor.second_connected_entity.get_data())->get_rigid_actor(), create_PxTransform(_base_descriptor.second_local_frame));

		created_joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, enable_collision);

		//created_joint->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		created_joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

		created_joint->setTolerance(0.02f);

		created_joint->setDistanceJointFlag(PxDistanceJointFlag::eSPRING_ENABLED, true);

		joint = created_joint;
		joint->userData = this;
	}

	DistanceJointComponent::~DistanceJointComponent()
	{
	}

	void DistanceJointComponent::set_damping(float damping)
	{
		using namespace physx;
		if (PxDistanceJoint* distance_joint = joint->is<PxDistanceJoint>())
		{
			distance_joint->setDamping(damping);
		}
	}

	float DistanceJointComponent::get_damping() const
	{
		using namespace physx;
		if (PxDistanceJoint* distance_joint = joint->is<PxDistanceJoint>())
		{
			return distance_joint->getDamping();
		}
		return 0.0f;
	}

	void DistanceJointComponent::set_stiffness(float stiffness)
	{
		using namespace physx;
		if (PxDistanceJoint* distance_joint = joint->is<PxDistanceJoint>())
		{
			return distance_joint->setStiffness(stiffness);
		}
	}

	float DistanceJointComponent::get_stiffness() const
	{
		using namespace physx;
		if (PxDistanceJoint* distance_joint = joint->is<PxDistanceJoint>())
		{
			return distance_joint->getStiffness();
		}
		return 0.0f;
	}

	void DistanceJointComponent::set_constraint_limit(float min_distance, float max_distance)
	{
		using namespace physx;
		if (PxDistanceJoint* distance_joint = joint->is<PxDistanceJoint>())
		{
			distance_joint->setDistanceJointFlag(PxDistanceJointFlag::eMIN_DISTANCE_ENABLED, true);
			distance_joint->setMinDistance(min_distance);

			distance_joint->setDistanceJointFlag(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED, true);
			distance_joint->setMinDistance(max_distance);
		}
	}
	
	SphericalJointComponent::SphericalJointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
		: JointComponent(_data, _base_descriptor)
	{
		using namespace physx;

		auto* physics = PhysicsHolder::physics_ref->get_physics();

		PxSphericalJoint* created_joint = physx::PxSphericalJointCreate(*physics,
			PhysicsUtils::get_body_component(base_descriptor.connected_entity.get_data())->get_rigid_actor(), create_PxTransform(_base_descriptor.local_frame),
			PhysicsUtils::get_body_component(base_descriptor.second_connected_entity.get_data())->get_rigid_actor(), create_PxTransform(_base_descriptor.second_local_frame));

		created_joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, enable_collision);

		//created_joint->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
		created_joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, true);

		joint = created_joint;
		joint->userData = this;
	}

	SphericalJointComponent::~SphericalJointComponent()
	{
	}

	void SphericalJointComponent::set_constraint_limit(float swing_y, float swing_z)
	{
		using namespace physx;
		if (PxSphericalJoint* spherical_joint = joint->is<PxSphericalJoint>())
		{
			spherical_joint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
			spherical_joint->setLimitCone(PxJointLimitCone{swing_y, swing_z});
		}
	}

	D6JointComponent::D6JointComponent(ref<Entity::EcsData> _data, const JointComponent::BaseDescriptor& _base_descriptor)
		: JointComponent(_data, _base_descriptor)
	{
		drive_transform.set_component(ComponentPtr(this));
		linear_drive_velocity.set_component(ComponentPtr(this));
		angular_drive_velocity.set_component(ComponentPtr(this));

		linear_drive_accelerated.set_component(ComponentPtr(this));
		twist_drive_accelerated.set_component(ComponentPtr(this));
		swing_drive_accelerated.set_component(ComponentPtr(this));
		slerp_drive_accelerated.set_component(ComponentPtr(this));

		linear_drive_stiffness.set_component(ComponentPtr(this));
		twist_drive_stiffness.set_component(ComponentPtr(this));
		swing_drive_stiffness.set_component(ComponentPtr(this));
		slerp_drive_stiffness.set_component(ComponentPtr(this));

		linear_drive_force_limit.set_component(ComponentPtr(this));
		twist_drive_force_limit.set_component(ComponentPtr(this));
		swing_drive_force_limit.set_component(ComponentPtr(this));
		slerp_drive_force_limit.set_component(ComponentPtr(this));

		twist_motion_type.set_component(ComponentPtr(this));
		swing_z_motion_type.set_component(ComponentPtr(this));
		swing_y_motion_type.set_component(ComponentPtr(this));

		linear_x_motion_type.set_component(ComponentPtr(this));
		linear_y_motion_type.set_component(ComponentPtr(this));
		linear_z_motion_type.set_component(ComponentPtr(this));

		swing_z_limit.set_component(ComponentPtr(this));
		swing_y_limit.set_component(ComponentPtr(this));

		twist_max_limit.set_component(ComponentPtr(this));
		twist_min_limit.set_component(ComponentPtr(this));
		linear_z_motion_type.set_component(ComponentPtr(this));
		linear_z_motion_type.set_component(ComponentPtr(this));

		linear_limit.set_component(ComponentPtr(this));
		distance_limit.set_component(ComponentPtr(this));

		drive_limits_are_forces.set_component(ComponentPtr(this));
		gpu_compatible.set_component(ComponentPtr(this));
	}

	D6JointComponent::~D6JointComponent()
	{
	}

}