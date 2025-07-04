#include "physics/core/joints_system.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/joint.h"
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

		registration::class_<JointsSystem>("JointsSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))
			.method("update", &JointsSystem::update)
			(metadata("update_group", update_types::PHYSICS), 
			 metadata("After", std::vector<std::string>{"PhysicsSystem::update"}));
	}

	JointsSystem::JointsSystem(World* _world)
		: System(_world)
	{
	}

	void JointsSystem::init()
	{
		entt::registry& registry = world->get_registry();
		registry.on_construct<D6JointComponent>().connect<&JointsSystem::on_d6_joint_created>(this);
	}

	void JointsSystem::update(float dt)
	{
		process_added_joints();

		sync_physics_to_component_changes();
	}

	void JointsSystem::sync_physics_to_component_changes()
	{
		using namespace physx;

		for (auto [entity_handle, observable_component, joint_component] : world->group(components_group<ObservableMemberChangedFlagComponent, D6JointComponent>).each())
		{
			PxJoint* joint = joint_component.get_native_joint();

			if (joint == nullptr)
			{
				continue;
			}

			PxD6Joint* native_joint = joint->is<PxD6Joint>();

			if (native_joint == nullptr)
			{
				continue;
			}

			if (joint_component.swing_y_limit.is_changed() ||
				joint_component.swing_z_limit.is_changed())
			{
				PxJointLimitCone limit(
					joint_component.swing_y_limit,
					joint_component.swing_z_limit,
					PxSpring(0.0f, 0.0f));
				limit.restitution = 0.0f;
				native_joint->setSwingLimit(limit);
			}

			if (joint_component.twist_min_limit.is_changed() ||
				joint_component.twist_max_limit.is_changed())
			{
				PxJointAngularLimitPair limit(
					joint_component.twist_min_limit,
					joint_component.twist_max_limit,
					PxSpring(0.0f, 0.0f));
				limit.restitution = 0.0f;

				native_joint->setTwistLimit(limit);
			}

			if (joint_component.linear_limit.is_changed())
			{
				native_joint->setLinearLimit(PxJointLinearLimit{ joint_component.linear_limit });
			}

			if (joint_component.distance_limit.is_changed())
			{
				native_joint->setDistanceLimit(PxJointLinearLimit{ joint_component.distance_limit });
			}

			if (joint_component.swing_y_motion_type.is_changed())
			{
				native_joint->setMotion(PxD6Axis::eSWING1, static_cast<PxD6Motion::Enum>(joint_component.swing_y_motion_type.get()));
			}
			if (joint_component.swing_z_motion_type.is_changed())
			{
				native_joint->setMotion(PxD6Axis::eSWING2, static_cast<PxD6Motion::Enum>(joint_component.swing_z_motion_type.get()));
			}
			if (joint_component.twist_motion_type.is_changed())
			{
				native_joint->setMotion(PxD6Axis::eTWIST, static_cast<PxD6Motion::Enum>(joint_component.twist_motion_type.get()));
			}
			if (joint_component.linear_x_motion_type.is_changed())
			{
				native_joint->setMotion(PxD6Axis::eX, static_cast<PxD6Motion::Enum>(joint_component.linear_x_motion_type.get()));
			}
			if (joint_component.linear_y_motion_type.is_changed())
			{
				native_joint->setMotion(PxD6Axis::eY, static_cast<PxD6Motion::Enum>(joint_component.linear_y_motion_type.get()));
			}
			if (joint_component.linear_z_motion_type.is_changed())
			{
				native_joint->setMotion(PxD6Axis::eZ, static_cast<PxD6Motion::Enum>(joint_component.linear_z_motion_type.get()));
			}

			if (joint_component.drive_limits_are_forces.is_changed())
			{
				native_joint->setConstraintFlag(PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES, joint_component.drive_limits_are_forces);
			}

			if (joint_component.gpu_compatible.is_changed())
			{
				native_joint->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, joint_component.gpu_compatible);
			}

			if (joint_component.twist_drive_accelerated.is_changed() ||
				joint_component.twist_drive_damping.is_changed() ||
				joint_component.twist_drive_stiffness.is_changed() ||
				joint_component.twist_drive_force_limit.is_changed())
			{
				PxD6JointDrive drive;
				drive.stiffness = joint_component.twist_drive_stiffness;
				drive.damping = joint_component.twist_drive_damping;
				drive.forceLimit = joint_component.twist_drive_force_limit;
				drive.flags = static_cast<PxD6JointDriveFlags>(joint_component.twist_drive_accelerated ? 1 : 0);
				native_joint->setDrive(PxD6Drive::eTWIST, drive);

				PxRigidActor* actor0 = nullptr;
				PxRigidActor* actor1 = nullptr;
				native_joint->getActors(actor0, actor1);

				if (PxRigidDynamic* actor_dynamic = actor0 == nullptr ? nullptr : actor0->is<PxRigidDynamic>(); actor_dynamic != nullptr)
				{
					if (!actor_dynamic->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION))
					{
						actor_dynamic->wakeUp();
					}
				}

				if (PxRigidDynamic* actor_dynamic = actor1 == nullptr ? nullptr : actor1->is<PxRigidDynamic>(); actor_dynamic != nullptr)
				{
					if (!actor_dynamic->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION))
					{
						actor_dynamic->wakeUp();
					}
				}
			}

			if (joint_component.swing_drive_accelerated.is_changed() ||
				joint_component.swing_drive_damping.is_changed() ||
				joint_component.swing_drive_stiffness.is_changed() ||
				joint_component.swing_drive_force_limit.is_changed())
			{
				PxD6JointDrive drive;
				drive.stiffness = joint_component.swing_drive_stiffness;
				drive.damping = joint_component.swing_drive_damping;
				drive.forceLimit = joint_component.swing_drive_force_limit;
				drive.flags = static_cast<PxD6JointDriveFlags>(joint_component.swing_drive_accelerated ? 1 : 0);
				native_joint->setDrive(PxD6Drive::eSWING, drive);

				PxRigidActor* actor0 = nullptr;
				PxRigidActor* actor1 = nullptr;
				native_joint->getActors(actor0, actor1);

				if (PxRigidDynamic* actor_dynamic = actor0 == nullptr ? nullptr : actor0->is<PxRigidDynamic>(); actor_dynamic != nullptr)
				{
					if (!actor_dynamic->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION))
					{
						actor_dynamic->wakeUp();
					}
				}

				if (PxRigidDynamic* actor_dynamic = actor1 == nullptr ? nullptr : actor1->is<PxRigidDynamic>(); actor_dynamic != nullptr)
				{
					if (!actor_dynamic->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION))
					{
						actor_dynamic->wakeUp();
					}
				}
			}

			if (joint_component.slerp_drive_accelerated.is_changed() ||
				joint_component.slerp_drive_damping.is_changed() ||
				joint_component.slerp_drive_stiffness.is_changed() ||
				joint_component.slerp_drive_force_limit.is_changed())
			{
				PxD6JointDrive drive;
				drive.stiffness = joint_component.slerp_drive_stiffness;
				drive.damping = joint_component.slerp_drive_damping;
				drive.forceLimit = joint_component.slerp_drive_force_limit;
				drive.flags = static_cast<PxD6JointDriveFlags>(joint_component.slerp_drive_accelerated ? 1 : 0);
				native_joint->setDrive(PxD6Drive::eSLERP, drive);

				PxRigidActor* actor0 = nullptr;
				PxRigidActor* actor1 = nullptr;
				native_joint->getActors(actor0, actor1);

				if (PxRigidDynamic* actor_dynamic = actor0 == nullptr ? nullptr : actor0->is<PxRigidDynamic>(); actor_dynamic != nullptr)
				{
					if (!actor_dynamic->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION))
					{
						actor_dynamic->wakeUp();
					}
				}

				if (PxRigidDynamic* actor_dynamic = actor1 == nullptr ? nullptr : actor1->is<PxRigidDynamic>(); actor_dynamic != nullptr)
				{
					if (!actor_dynamic->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION))
					{
						actor_dynamic->wakeUp();
					}
				}
			}

			if (joint_component.linear_drive_accelerated.is_changed() ||
				joint_component.linear_drive_damping.is_changed() ||
				joint_component.linear_drive_stiffness.is_changed() ||
				joint_component.linear_drive_force_limit.is_changed())
			{
				PxD6JointDrive drive;
				drive.stiffness = joint_component.linear_drive_stiffness;
				drive.damping = joint_component.linear_drive_damping;
				drive.forceLimit = joint_component.linear_drive_force_limit;
				drive.flags = static_cast<PxD6JointDriveFlags>(joint_component.linear_drive_accelerated ? 1 : 0);
				native_joint->setDrive(PxD6Drive::eX, drive);
				native_joint->setDrive(PxD6Drive::eY, drive);
				native_joint->setDrive(PxD6Drive::eZ, drive);

				PxRigidActor* actor0 = nullptr;
				PxRigidActor* actor1 = nullptr;
				native_joint->getActors(actor0, actor1);

				if (PxRigidDynamic* actor_dynamic = actor0 == nullptr ? nullptr : actor0->is<PxRigidDynamic>(); actor_dynamic != nullptr)
				{
					if (!actor_dynamic->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION))
					{
						actor_dynamic->wakeUp();
					}
				}

				if (PxRigidDynamic* actor_dynamic = actor1 == nullptr ? nullptr : actor1->is<PxRigidDynamic>(); actor_dynamic != nullptr)
				{
					if (!actor_dynamic->getActorFlags().isSet(PxActorFlag::eDISABLE_SIMULATION))
					{
						actor_dynamic->wakeUp();
					}
				}
			}

			if (joint_component.angular_drive_velocity.is_changed() || 
				joint_component.linear_drive_velocity.is_changed())
			{
				native_joint->setDriveVelocity(create_PxVec3(joint_component.linear_drive_velocity), create_PxVec3(joint_component.angular_drive_velocity), true);
			}

			if (joint_component.drive_transform.is_changed())
			{
				native_joint->setDrivePosition(create_PxTransform(joint_component.drive_transform), true);
			}
		}
	}

	void JointsSystem::process_added_joints()
	{
		using namespace physx;

		ScopedSpinLock _lock{ sync };

		for (auto iter = d6_joints_to_init.begin(); iter != d6_joints_to_init.end();)
		{
			Entity entity = world->get_entity(*iter);

			D6JointComponent& joint_component = entity.get_component<D6JointComponent>();

			PxRigidActor* first_actor = PhysicsUtils::get_body_component(joint_component.base_descriptor.connected_entity.get())->get_rigid_actor();
			PxRigidActor* second_actor = PhysicsUtils::get_body_component(joint_component.base_descriptor.second_connected_entity.get())->get_rigid_actor();

			if (first_actor == nullptr ||
				second_actor == nullptr)
			{
				++iter;
				continue;
			}

			PxD6Joint* created_joint = PxD6JointCreate(*PhysicsHolder::physics_ref->get_physics(),
				first_actor,
				create_PxTransform(joint_component.base_descriptor.local_frame),
				second_actor,
				create_PxTransform(joint_component.base_descriptor.second_local_frame));

			created_joint->setConstraintFlag(PxConstraintFlag::eVISUALIZATION, false);
			created_joint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, joint_component.enable_collision);

			if (PhysicsHolder::physics_ref->is_gpu())
			{
				created_joint->setConstraintFlag(PxConstraintFlag::eGPU_COMPATIBLE, true);
			}

			joint_component.joint = created_joint;
			joint_component.joint->userData = this;

			joint_component.state = JointComponent::State::ENABLED;

			iter = d6_joints_to_init.erase(iter);
		}
	}

	void JointsSystem::on_d6_joint_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ sync };

		d6_joints_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}

}
