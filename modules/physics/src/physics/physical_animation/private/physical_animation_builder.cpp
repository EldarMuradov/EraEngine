#include "physics/physical_animation/physical_animation_builder.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/shape_component.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"
#include "physics/physical_animation/physical_animation_component.h"
#include "physics/joint.h"

#include <core/math.h>
#include "core/debug/debug_var.h"

#include <ecs/base_components/transform_component.h>
#include <ecs/world.h>

namespace era_engine::physics
{
	static DebugVar<bool> enable_spring_pelvis_attachment = DebugVar<bool>("physics.physical_animation.enable_spring_pelvis_attachment", false);

	static Entity create_child_entity(Entity parent, 
		std::string_view entity_name,
		const ConstraintDetails& details,
		uint32 joint_id = INVALID_JOINT, 
		PhysicalAnimationLimbComponent::Type type = PhysicalAnimationLimbComponent::Type::BODY_UPPER)
	{
		Entity child = parent.get_world()->create_entity(entity_name.data());
		child.set_parent(parent.get_handle());

		PhysicalAnimationLimbComponent* limb_component = child.add_component<PhysicalAnimationLimbComponent>();
		limb_component->joint_id = joint_id;
		limb_component->type = type;
		limb_component->stiffness = details.drive_stiffness;
		limb_component->damping = details.drive_damping;
		limb_component->drive_velocity_modifier = details.drive_velocity_modifier;

		PhysicalAnimationComponent* physical_animation_component = parent.get_component<PhysicalAnimationComponent>();
		physical_animation_component->limbs.push_back(EntityPtr{ child });

		return child;
	}

	static DynamicBodyComponent* create_dynamic_body(
		Entity& entity,
		const float mass,
		const float max_contact_impulse = std::numeric_limits<float>::max(),
		const float max_angular_velocity = std::numeric_limits<float>::max())
	{
		DynamicBodyComponent* dynamic_body_component = entity.add_component<DynamicBodyComponent>();
		dynamic_body_component->mass = mass;
		dynamic_body_component->ccd = true;
		dynamic_body_component->use_gravity = false;
		dynamic_body_component->simulated = false;
		dynamic_body_component->linear_damping = 0.1f;
		dynamic_body_component->angular_damping = 0.25f;
		dynamic_body_component->max_angular_velocity = max_angular_velocity;
		dynamic_body_component->max_contact_impulse = max_contact_impulse;
		dynamic_body_component->solver_position_iterations_count = 32;
		dynamic_body_component->solver_velocity_iterations_count = 16;
		dynamic_body_component->sleep_threshold = 0.01f;
		dynamic_body_component->stabilization_threshold = 0.01f;

		return dynamic_body_component;
	}

	static void create_d6_joint(
		const Entity& source,
		const ConstraintDetails& details,
		Entity& e0, 
		Entity& e1,
		const trs& e0_joint_transform,
		const trs& e1_joint_transform,
		const trs& e0_local_transform,
		const trs& e1_local_transform,
		float twist_min_deg,
		float twist_max_deg,
		float swing_y_deg,
		float swing_z_deg,
		bool accelerated_drive = true,
		bool enable_slerp_drive = false)
	{
		const trs e0_to_e0_joint_transform = invert(e0_local_transform) * e0_joint_transform;
		const trs e1_to_e1_joint_transform = invert(e1_local_transform) * e1_joint_transform;

		JointComponent::BaseDescriptor descriptor;
		descriptor.connected_entity = e0.get_data_weakref();
		descriptor.second_connected_entity = e1.get_data_weakref();
		descriptor.local_frame = e0_to_e0_joint_transform;
		descriptor.second_local_frame = e1_to_e1_joint_transform;

		Entity joint_entity = e0.get_world()->create_entity();
		joint_entity.set_parent(e0.get_handle());

		e0.get_component<PhysicalAnimationLimbComponent>()->joint_entity_ptr = EntityPtr{ joint_entity };

		D6JointComponent* joint_component = joint_entity.add_component<D6JointComponent>(descriptor);

		joint_component->enable_collision = false;

		e1.get_component<PhysicalAnimationLimbComponent>()->parent_joint_component = ComponentPtr{ joint_component };
	
		if (enable_slerp_drive)
		{
			joint_component->slerp_drive_damping = details.drive_damping;
			joint_component->slerp_drive_force_limit = details.max_force;
			joint_component->slerp_drive_stiffness = details.drive_stiffness;
			joint_component->slerp_drive_accelerated = accelerated_drive;
		}
		else
		{
			joint_component->swing_drive_force_limit = details.max_force;
			joint_component->swing_drive_accelerated = accelerated_drive;
			joint_component->swing_drive_stiffness = details.drive_stiffness;
			joint_component->swing_drive_damping = details.drive_damping;
						
			joint_component->twist_drive_stiffness = details.drive_stiffness;
			joint_component->twist_drive_damping = details.drive_damping;
			joint_component->twist_drive_force_limit = details.max_force;
			joint_component->twist_drive_accelerated = accelerated_drive;
		}

		if (fuzzy_equals(twist_max_deg - twist_min_deg, 0.0f))
		{
			joint_component->twist_motion_type = D6JointComponent::Motion::LOCKED;
		}
		else
		{
			joint_component->twist_motion_type = D6JointComponent::Motion::LIMITED;
			joint_component->twist_min_limit = deg2rad(twist_min_deg);
			joint_component->twist_max_limit = deg2rad(twist_max_deg);
		}

		if (fuzzy_equals(swing_y_deg, 0.0f))
		{
			joint_component->swing_y_motion_type = D6JointComponent::Motion::LOCKED;
		}
		else
		{
			joint_component->swing_y_motion_type = D6JointComponent::Motion::LIMITED;
			joint_component->swing_y_limit = deg2rad(swing_y_deg);
		}

		if (fuzzy_equals(swing_z_deg, 0.0f))
		{
			joint_component->swing_z_motion_type = D6JointComponent::Motion::LOCKED;
		}
		else
		{
			joint_component->swing_z_motion_type = D6JointComponent::Motion::LIMITED;
			joint_component->swing_z_limit = deg2rad(swing_z_deg);
		}
	}

	// Return bottom of the box, used for creating d6s
	// Bottom = lowest point along length axis.
	static trs position_box_between_joints(
		BoxShapeComponent* box_shape_component,
		const vec3& size,
		const trs& from_joint_transform,
		const trs& to_joint_transform,
		const trs& owner_joint_transform)
	{
		box_shape_component->half_extents = (size / 2.0f) / owner_joint_transform.scale.x;

		const vec3 offset = to_joint_transform.position - from_joint_transform.position;
		const vec3 center_between_joints = from_joint_transform.position + offset / 2.0f;
		const vec3 direction = normalize(offset);

		const trs box_transform(
			center_between_joints,
			shortest_arc(vec3(1.0f, 0.0f, 0.0f), direction),
			vec3(1.0f));

		const trs box_transform_relative_to_owner = invert(owner_joint_transform) *  box_transform;

		box_shape_component->local_position = box_transform_relative_to_owner.position;
		box_shape_component->local_rotation = box_transform_relative_to_owner.rotation;

		return trs(box_transform.position - direction * size.x / 2.0f, box_transform.rotation, box_transform.scale);
	}

	// Return bottom of the capsule, used for creating d6s
	// Bottom = lowest point on x axis.
	static trs position_capsule_between_joints_from_radius(
		CapsuleShapeComponent* capsule_shape_component,
		const float radius,
		const trs& from_joint_transform,
		const trs& to_joint_transform,
		const trs& owner_joint_transform,
		const float height_multiplier)
	{
		const vec3 offset = to_joint_transform.position - from_joint_transform.position;
		const float height = (length(offset) - 2 * radius) * height_multiplier;

		capsule_shape_component->radius = radius / owner_joint_transform.scale.x;
		capsule_shape_component->half_height = (height / 2.0f) / owner_joint_transform.scale.x;

		const vec3 center_between_joints = from_joint_transform.position + offset / 2.0f;
		const vec3 direction = normalize(offset);

		const trs capsule_transform(
			center_between_joints,
			shortest_arc(vec3(1.0f, 0.0f, 0.0f), direction),
			vec3(1.0f));

		const trs capsule_transform_relative_to_owner = invert(owner_joint_transform) * capsule_transform;

		capsule_shape_component->local_position = capsule_transform_relative_to_owner.position;
		capsule_shape_component->local_rotation = capsule_transform_relative_to_owner.rotation;

		return trs(capsule_transform.position - direction * (radius + height / 2.0f), capsule_transform.rotation, capsule_transform.scale);
	}

	// Return bottom of the capsule, used for creating d6s
	// Bottom = lowest point on x axis.
	static trs position_capsule(
		CapsuleShapeComponent* capsule_shape_component,
		const float radius,
		const float half_height,
		const trs& from_joint_transform,
		const trs& to_joint_transform,
		const trs& owner_joint_transform,
		const vec3& offset_for_from_joint = vec3::zero)
	{
		const vec3 offset = to_joint_transform.position - from_joint_transform.position;

		capsule_shape_component->radius = radius / owner_joint_transform.scale.x;
		capsule_shape_component->half_height = half_height / owner_joint_transform.scale.x;

		const vec3 center_between_joints = from_joint_transform.position + offset / 2.0f + offset_for_from_joint;
		const vec3 direction = normalize(offset);

		const trs capsule_transform(
			center_between_joints,
			shortest_arc(vec3(1.0f, 0.0f, 0.0f), direction),
			vec3(1.0f));

		const trs capsule_transform_relative_to_owner = invert(owner_joint_transform) * capsule_transform;

		capsule_shape_component->local_position = capsule_transform_relative_to_owner.position;
		capsule_shape_component->local_rotation = capsule_transform_relative_to_owner.rotation;

		return trs(capsule_transform.position - direction * (radius + half_height), capsule_transform.rotation, capsule_transform.scale);
	}

	// Return bottom of the capsule, used for creating d6s
	// Bottom = x is along the diameter, lowest point on x axis.
	static trs position_capsule_between_joints_from_height(
		CapsuleShapeComponent* capsule_shape_component,
		float height,
		const trs& from_joint_transform,
		const trs& to_joint_transform,
		const trs& owner_joint_transform,
		const float radius_modifier = 1.0f)
	{
		const vec3 offset = to_joint_transform.position - from_joint_transform.position;
		const float radius = length(offset) * 0.65f * radius_modifier; // Increased intentionally for thorax, abdomen and pelvis. Should probably be specified outside.
		if (height - 2.0f * radius > EPSILON)
		{
			height -= 2.0f * radius;
		}

		capsule_shape_component->radius = radius / owner_joint_transform.scale.x;
		capsule_shape_component->half_height = (height / 2.0f) / owner_joint_transform.scale.x;

		const vec3 center_between_joints = from_joint_transform.position + offset / 2.0f;
		const vec3 direction = normalize(offset);

		const trs capsule_transform(
			center_between_joints,
			shortest_arc(vec3(0.0f, 1.0f, 0.0f), direction),
			vec3(1.0f));
		const trs capsule_transform_relative_to_owner = invert(owner_joint_transform) * capsule_transform;

		capsule_shape_component->local_position = capsule_transform_relative_to_owner.position;
		capsule_shape_component->local_rotation = capsule_transform_relative_to_owner.rotation;

		const vec3 capsule_x_axis = capsule_transform.rotation * vec3(1.0f, 0.0f, 0.0f);
		return trs(capsule_transform.position - direction * radius, capsule_transform.rotation, capsule_transform.scale) * trs(vec3::zero, shortest_arc(capsule_x_axis, direction), vec3(1.0f));
	}

	void PhysicalAnimationBuilderUtils::build_simulated_body(Entity ragdoll)
	{
		using namespace animation;

		SkeletonComponent* skeleton_component = ragdoll.get_component<SkeletonComponent>();

		const Skeleton* skeleton = skeleton_component->skeleton;
		if (skeleton == nullptr)
		{
			ASSERT(false);
			return;
		}

		PhysicalAnimationComponent* physical_animation_component = ragdoll.get_component<PhysicalAnimationComponent>();
		const RagdollSettings& settings = physical_animation_component->settings;
		const RagdollJointIds& joint_init_ids = physical_animation_component->joint_init_ids;

		ref<RagdollProfile> ragdoll_profile = physical_animation_component->get_ragdoll_profile();

		const trs& ragdoll_world_transform = ragdoll.get_component<TransformComponent>()->get_world_transform();

		const ConstraintDetails& head_constraint = ragdoll_profile->head_constraint;
		const ConstraintDetails& neck_constraint = ragdoll_profile->neck_constraint;
		const ConstraintDetails& body_upper_constraint = ragdoll_profile->body_upper_constraint;
		const ConstraintDetails& body_middle_constraint = ragdoll_profile->body_middle_constraint;
		const ConstraintDetails& arm_constraint = ragdoll_profile->arm_constraint;
		const ConstraintDetails& forearm_constraint = ragdoll_profile->forearm_constraint;
		const ConstraintDetails& hand_constraint = ragdoll_profile->hand_constraint;
		const ConstraintDetails& leg_constraint = ragdoll_profile->leg_constraint;
		const ConstraintDetails& calf_constraint = ragdoll_profile->calf_constraint;
		const ConstraintDetails& foot_constraint = ragdoll_profile->foot_constraint;

		const float mass = physical_animation_component->mass;

		trs neck_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.neck_idx);
		neck_joint_transform.position = neck_joint_transform.position + settings.neck_joint_adjastment;

		trs head_end_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.head_end_idx);
		head_end_joint_transform.position = head_end_joint_transform.position + settings.head_joint_adjastment;

		trs head_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.head_idx);
		head_joint_transform.position = head_joint_transform.position + settings.head_joint_adjastment;

		trs thorax_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.spine_03_idx);
		thorax_joint_transform.position = thorax_joint_transform.position + settings.thorax_joint_adjastment;

		trs abdomen_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.spine_01_idx);
		abdomen_joint_transform.position = abdomen_joint_transform.position + settings.abdomen_joint_adjastment;

		trs pelvis_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.pelvis_idx);
		pelvis_joint_transform.position = pelvis_joint_transform.position + settings.pelvis_joint_adjastment;

		const trs left_arm_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.upperarm_l_idx);
		const trs left_forearm_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.lowerarm_l_idx);
		const trs left_hand_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.hand_l_idx);
		const trs left_hand_end_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.hand_end_l_idx);

		const trs right_arm_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.upperarm_r_idx);
		const trs right_forearm_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.lowerarm_r_idx);
		const trs right_hand_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.hand_r_idx);
		const trs right_hand_end_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.hand_end_r_idx);

		const trs left_up_leg_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.thigh_l_idx);
		const trs left_leg_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.calf_l_idx);
		const trs left_foot_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.foot_l_idx);
		const trs left_foot_end_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.foot_end_l_idx);

		const trs right_up_leg_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.thigh_r_idx);
		const trs right_leg_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.calf_r_idx);
		const trs right_foot_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.foot_r_idx);
		const trs right_foot_end_joint_transform = SkeletonUtils::get_object_space_joint_transform(skeleton, joint_init_ids.foot_end_r_idx);

		// Lower body goes from pelvis to middle, and upper body goes from middle to thorax
		const trs middle_abdomen_thorax_transform = trs(
			lerp(abdomen_joint_transform.position, thorax_joint_transform.position, 0.5f),
			slerp(abdomen_joint_transform.rotation, thorax_joint_transform.rotation, 0.5f),
			lerp(abdomen_joint_transform.scale, thorax_joint_transform.scale, 0.5f));

		const float distance_between_arms = length(left_arm_joint_transform.position - right_arm_joint_transform.position);
		const float distance_between_hand_and_hand_end = length(left_hand_joint_transform.position - left_hand_end_joint_transform.position);

		const float distance_between_foot_and_foot_end = length(left_foot_joint_transform.position - left_foot_end_joint_transform.position);
		const float distance_between_foot_y_and_foot_end_y = abs(left_foot_joint_transform.position.y - left_foot_end_joint_transform.position.y);

		ref<PhysicsMaterial> material = PhysicsHolder::physics_ref->create_material(0.1f, 0.8f, 0.8f);
		ASSERT(material != nullptr);

		Entity head;
		trs head_capsule_bottom_transform;

		Entity body_upper;
		trs body_upper_capsule_bottom_transform;

		Entity body_lower;
		trs lower_default_transform;

		Entity left_arm;
		trs left_arm_capsule_bottom_transform;

		Entity left_forearm;
		trs left_forearm_capsule_bottom_transform;

		Entity left_hand;
		trs left_hand_box_bottom_transform;

		Entity right_arm;
		trs right_arm_capsule_bottom_transform;

		Entity right_forearm;
		trs right_forearm_capsule_bottom_transform;

		Entity right_hand;
		trs right_hand_box_bottom_transform;

		Entity left_up_leg;
		trs left_up_leg_capsule_bottom_transform;

		Entity left_leg;
		trs left_leg_capsule_bottom_transform;

		Entity left_foot;
		trs left_foot_box_bottom_transform;

		Entity right_up_leg;
		trs right_up_leg_capsule_bottom_transform;

		Entity right_leg;
		trs right_leg_capsule_bottom_transform;

		Entity right_foot;
		trs right_foot_box_bottom_transform;

		// Head
		{
			head = create_child_entity(ragdoll, "head", head_constraint, joint_init_ids.head_idx, PhysicalAnimationLimbComponent::Type::HEAD);

			TransformComponent* transform_component = head.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * neck_joint_transform);

			trs head_end_transform = trs(head_joint_transform.position + settings.head_end_joint_adjastment, head_joint_transform.rotation, head_joint_transform.scale);

			CapsuleShapeComponent* capsule_shape_component = head.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			head_capsule_bottom_transform = position_capsule_between_joints_from_radius(
				capsule_shape_component,
				settings.head_radius,
				neck_joint_transform,
				head_end_transform,
				neck_joint_transform,
				1.0f);

			create_dynamic_body(head, mass * settings.head_mass_percentage, settings.max_head_contact_impulse);
		}

		// Body upper (from body middle to neck)
		{
			body_upper = create_child_entity(ragdoll, "body_upper", body_upper_constraint, joint_init_ids.spine_03_idx, PhysicalAnimationLimbComponent::Type::BODY_UPPER);

			TransformComponent* transform_component = body_upper.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * thorax_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = body_upper.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			body_upper_capsule_bottom_transform = position_capsule_between_joints_from_height(
				capsule_shape_component,
				distance_between_arms * settings.upper_body_height_modifier,
				middle_abdomen_thorax_transform,
				neck_joint_transform,
				thorax_joint_transform,
				settings.upper_body_radius_modifier);

			create_dynamic_body(body_upper, mass * settings.body_upper_mass_percentage, settings.max_body_contact_impulse);
		}

		// Body lower (from pelvis to abdomen)
		{
			body_lower = create_child_entity(ragdoll, "pelvis", body_middle_constraint, joint_init_ids.pelvis_idx, PhysicalAnimationLimbComponent::Type::BODY_LOWER);

			TransformComponent* transform_component = body_lower.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * pelvis_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = body_lower.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			lower_default_transform = position_capsule_between_joints_from_height(
				capsule_shape_component,
				distance_between_arms * settings.lower_body_height_modifier,
				pelvis_joint_transform,
				middle_abdomen_thorax_transform,
				pelvis_joint_transform,
				settings.lower_body_radius_modifier);

			create_dynamic_body(body_lower, mass * settings.body_lower_mass_percentage, settings.max_body_contact_impulse);
		}

		// Lower body attachment.
		{
			Entity body_lower_ghost = ragdoll.get_world()->create_entity("pelvis_ghost");
			body_lower_ghost.set_parent(ragdoll.get_handle());

			TransformComponent* transform_component = body_lower_ghost.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * pelvis_joint_transform);

			DynamicBodyComponent* lower_body_ghost_component = body_lower_ghost.add_component<DynamicBodyComponent>();
			lower_body_ghost_component->simulated = true;
			lower_body_ghost_component->kinematic = true;
			lower_body_ghost_component->use_gravity = false;

			BoxShapeComponent* box_shape_component = body_lower_ghost.add_component<BoxShapeComponent>();
			box_shape_component->collision_type = CollisionType::NONE;
			box_shape_component->material = material;
			box_shape_component->half_extents = vec3(0.01f);

			JointComponent::BaseDescriptor descriptor;
			descriptor.connected_entity = body_lower_ghost.get_data_weakref();
			descriptor.second_connected_entity = body_lower.get_data_weakref();
			
			if (enable_spring_pelvis_attachment)
			{
				DistanceJointComponent* joint_component = body_lower_ghost.add_component<DistanceJointComponent>(descriptor);
				joint_component->enable_collision = false;
				joint_component->max_distance = 0.05f;
				joint_component->min_distance = 0.001f;
				joint_component->stiffness = 1500.0f;
				joint_component->damping = 40.0f;
				joint_component->spring_enabled = true;
			}
			else
			{
				FixedJointComponent* joint_component = body_lower_ghost.add_component<FixedJointComponent>(descriptor);
				joint_component->enable_collision = false;
			}
			physical_animation_component->use_spring_pelvis_attachment = enable_spring_pelvis_attachment;
			physical_animation_component->attachment_body = EntityPtr{ body_lower_ghost };
		}

		// Left arm
		{
			left_arm = create_child_entity(ragdoll, "l_arm", arm_constraint, joint_init_ids.upperarm_l_idx, PhysicalAnimationLimbComponent::Type::ARM);

			TransformComponent* transform_component = left_arm.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * left_arm_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = left_arm.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			left_arm_capsule_bottom_transform = position_capsule_between_joints_from_radius(
				capsule_shape_component,
				settings.arm_radius,
				left_arm_joint_transform,
				left_forearm_joint_transform,
				left_arm_joint_transform,
				1.5f);

			create_dynamic_body(left_arm, mass * settings.arm_mass_percentage, settings.max_arm_contact_impulse);
		}

		// Left forearm
		{
			left_forearm = create_child_entity(ragdoll, "l_forearm", forearm_constraint, joint_init_ids.lowerarm_l_idx, PhysicalAnimationLimbComponent::Type::FOREARM);

			TransformComponent* transform_component = left_forearm.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * left_forearm_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = left_forearm.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			left_forearm_capsule_bottom_transform = position_capsule_between_joints_from_radius(
				capsule_shape_component,
				settings.forearm_radius,
				left_forearm_joint_transform,
				left_hand_joint_transform,
				left_forearm_joint_transform,
				1.5f);

			create_dynamic_body(left_forearm, mass * settings.forearm_mass_percentage, settings.max_forearm_contact_impulse);
		}

		// Left hand
		{
			left_hand = create_child_entity(ragdoll, "l_hand", hand_constraint, joint_init_ids.hand_l_idx, PhysicalAnimationLimbComponent::Type::HAND);

			TransformComponent* transform_component = left_hand.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * left_hand_joint_transform);

			BoxShapeComponent* box_shape_component = left_hand.add_component<BoxShapeComponent>();
			box_shape_component->collision_type = CollisionType::NONE;
			box_shape_component->material = material;

			left_hand_box_bottom_transform = position_box_between_joints(
				box_shape_component,
				vec3(distance_between_hand_and_hand_end, settings.hand_height, settings.hand_width),
				left_hand_joint_transform,
				left_hand_end_joint_transform,
				left_hand_joint_transform);

			create_dynamic_body(left_hand, mass * settings.hand_mass_percentage, settings.max_hand_contact_impulse);
		}

		// Right arm
		{
			right_arm = create_child_entity(ragdoll, "r_arm", arm_constraint, joint_init_ids.upperarm_r_idx, PhysicalAnimationLimbComponent::Type::ARM);

			TransformComponent* transform_component = right_arm.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * right_arm_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = right_arm.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			right_arm_capsule_bottom_transform = position_capsule_between_joints_from_radius(
				capsule_shape_component,
				settings.arm_radius,
				right_arm_joint_transform,
				right_forearm_joint_transform,
				right_arm_joint_transform,
				1.5f);

			create_dynamic_body(right_arm, mass * settings.arm_mass_percentage, settings.max_arm_contact_impulse);
		}

		// Right forearm
		{
			right_forearm = create_child_entity(ragdoll, "r_forearm", forearm_constraint, joint_init_ids.lowerarm_r_idx, PhysicalAnimationLimbComponent::Type::FOREARM);

			TransformComponent* transform_component = right_forearm.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * right_forearm_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = right_forearm.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			right_forearm_capsule_bottom_transform = position_capsule_between_joints_from_radius(
				capsule_shape_component,
				settings.forearm_radius,
				right_forearm_joint_transform,
				right_hand_joint_transform,
				right_forearm_joint_transform,
				1.5f);

			create_dynamic_body(right_forearm, mass * settings.forearm_mass_percentage, settings.max_forearm_contact_impulse);
		}

		// Right hand
		{
			right_hand = create_child_entity(ragdoll, "r_hand", hand_constraint, joint_init_ids.hand_r_idx, PhysicalAnimationLimbComponent::Type::HAND);

			TransformComponent* transform_component = right_hand.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * right_hand_joint_transform);

			BoxShapeComponent* box_shape_component = right_hand.add_component<BoxShapeComponent>();
			box_shape_component->collision_type = CollisionType::NONE;
			box_shape_component->material = material;

			right_hand_box_bottom_transform = position_box_between_joints(
				box_shape_component,
				vec3(distance_between_hand_and_hand_end, settings.hand_height, settings.hand_width),
				right_hand_joint_transform,
				right_hand_end_joint_transform,
				right_hand_joint_transform);

			create_dynamic_body(right_hand, mass * settings.hand_mass_percentage, settings.max_hand_contact_impulse);
		}

		// Left up leg
		{
			left_up_leg = create_child_entity(ragdoll, "l_thigh", leg_constraint, joint_init_ids.thigh_l_idx, PhysicalAnimationLimbComponent::Type::LEG);

			TransformComponent* transform_component = left_up_leg.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * left_up_leg_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = left_up_leg.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			left_up_leg_capsule_bottom_transform = position_capsule_between_joints_from_radius(
				capsule_shape_component,
				settings.up_leg_radius,
				left_up_leg_joint_transform,
				left_leg_joint_transform,
				left_up_leg_joint_transform,
				0.9f);

			create_dynamic_body(left_up_leg, mass * settings.up_leg_mass_percentage, settings.max_up_leg_contact_impulse);
		}

		// Left leg
		{
			left_leg = create_child_entity(ragdoll, "l_calf", calf_constraint, joint_init_ids.calf_l_idx, PhysicalAnimationLimbComponent::Type::CALF);

			TransformComponent* transform_component = left_leg.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * left_leg_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = left_leg.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			left_leg_capsule_bottom_transform = position_capsule_between_joints_from_radius(
				capsule_shape_component,
				settings.leg_radius,
				left_leg_joint_transform,
				left_foot_joint_transform,
				left_leg_joint_transform,
				1.3f);

			create_dynamic_body(left_leg, mass * settings.leg_mass_percentage, settings.max_leg_contact_impulse);
		}

		// Left foot
		{
			const vec3 foot_end_same_y(left_foot_end_joint_transform.position.x, left_foot_joint_transform.position.y, left_foot_end_joint_transform.position.z);
			const vec3 foot_to_foot_end_offset = foot_end_same_y - left_foot_joint_transform.position;
			const vec3 center = left_foot_joint_transform.position + foot_to_foot_end_offset / 2.0f - vec3(0.0f, distance_between_foot_y_and_foot_end_y / 2.0f, 0.0f);

			left_foot = create_child_entity(ragdoll, "l_foot", foot_constraint, joint_init_ids.foot_l_idx, PhysicalAnimationLimbComponent::Type::FOOT);

			TransformComponent* transform_component = left_foot.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * left_foot_joint_transform);

			BoxShapeComponent* box_shape_component = left_foot.add_component<BoxShapeComponent>();
			box_shape_component->collision_type = CollisionType::NONE;
			box_shape_component->material = material;

			left_foot_box_bottom_transform = position_box_between_joints(
				box_shape_component,
				vec3(distance_between_foot_and_foot_end, distance_between_foot_y_and_foot_end_y * 2.0f, settings.foot_width),
				trs(center - foot_to_foot_end_offset / 2.0f, left_foot_joint_transform.rotation, left_foot_joint_transform.scale),
				trs(center + foot_to_foot_end_offset / 2.0f, left_foot_joint_transform.rotation, left_foot_joint_transform.scale),
				left_foot_joint_transform);

			create_dynamic_body(left_foot, mass * settings.foot_mass_percentage, settings.max_foot_contact_impulse);
		}

		// Right up leg
		{
			right_up_leg = create_child_entity(ragdoll, "r_thigh", leg_constraint, joint_init_ids.thigh_r_idx, PhysicalAnimationLimbComponent::Type::LEG);

			TransformComponent* transform_component = right_up_leg.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * right_up_leg_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = right_up_leg.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			right_up_leg_capsule_bottom_transform = position_capsule_between_joints_from_radius(
				capsule_shape_component,
				settings.up_leg_radius,
				right_up_leg_joint_transform,
				right_leg_joint_transform,
				right_up_leg_joint_transform,
				0.9f);

			create_dynamic_body(right_up_leg, mass * settings.up_leg_mass_percentage, settings.max_up_leg_contact_impulse);
		}

		// Right leg
		{
			right_leg = create_child_entity(ragdoll, "r_calf", calf_constraint, joint_init_ids.calf_r_idx, PhysicalAnimationLimbComponent::Type::CALF);

			TransformComponent* transform_component = right_leg.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * right_leg_joint_transform);

			CapsuleShapeComponent* capsule_shape_component = right_leg.add_component<CapsuleShapeComponent>();
			capsule_shape_component->collision_type = CollisionType::RAGDOLL;
			capsule_shape_component->material = material;

			right_leg_capsule_bottom_transform = position_capsule_between_joints_from_radius(
				capsule_shape_component,
				settings.leg_radius,
				right_leg_joint_transform,
				right_foot_joint_transform,
				right_leg_joint_transform,
				1.3f);

			create_dynamic_body(right_leg, mass * settings.leg_mass_percentage, settings.max_leg_contact_impulse);
		}

		// Right foot
		{
			const vec3 foot_end_same_y(right_foot_end_joint_transform.position.x, right_foot_joint_transform.position.y, right_foot_end_joint_transform.position.z);
			const vec3 foot_to_foot_end_offset = foot_end_same_y - right_foot_joint_transform.position;
			const vec3 center = right_foot_joint_transform.position + foot_to_foot_end_offset / 2.0f - vec3(0.0f, distance_between_foot_y_and_foot_end_y / 2.0f, 0.0f);

			right_foot = create_child_entity(ragdoll, "r_foot", foot_constraint, joint_init_ids.foot_r_idx, PhysicalAnimationLimbComponent::Type::FOOT);

			TransformComponent* transform_component = right_foot.get_component<TransformComponent>();
			transform_component->set_world_transform(ragdoll_world_transform * right_foot_joint_transform);

			BoxShapeComponent* box_shape_component = right_foot.add_component<BoxShapeComponent>();
			box_shape_component->collision_type = CollisionType::RAGDOLL;
			box_shape_component->material = material;

			right_foot_box_bottom_transform = position_box_between_joints(
				box_shape_component,
				vec3(distance_between_foot_and_foot_end, distance_between_foot_y_and_foot_end_y * 2.0f, settings.foot_width),
				trs(center - foot_to_foot_end_offset / 2.0f, right_foot_joint_transform.rotation, right_foot_joint_transform.scale),
				trs(center + foot_to_foot_end_offset / 2.0f, right_foot_joint_transform.rotation, right_foot_joint_transform.scale),
				right_foot_joint_transform);

			create_dynamic_body(right_foot, mass * settings.foot_mass_percentage, settings.max_foot_contact_impulse);
		}

		// Neck -> head
		create_d6_joint(
			ragdoll,
			head_constraint,
			body_upper, head,
			head_capsule_bottom_transform,
			head_capsule_bottom_transform,
			thorax_joint_transform,
			head_joint_transform,
			-45.0f, 45.0f,
			40.0f, 40.0f);

		// Body lower -> body upper
		const float body_upper_forward_angle_deg = 25.0f;
		const float body_upper_backward_angle_deg = 10.0f;
		const float body_upper_d6_swing_y_deg = (body_upper_forward_angle_deg + body_upper_backward_angle_deg) / 2.0f;
		const vec3 body_upper_capsule_y_axis = body_upper_capsule_bottom_transform.rotation * vec3(0.0f, 1.0f, 0.0f);
		const trs body_middle_d6_transform = trs(
			body_upper_capsule_bottom_transform.position,
			quat(body_upper_capsule_y_axis, deg2rad(body_upper_d6_swing_y_deg - body_upper_backward_angle_deg)) * body_upper_capsule_bottom_transform.rotation,
			body_upper_capsule_bottom_transform.scale);
		create_d6_joint(
			ragdoll,
			body_upper_constraint,
			body_lower, body_upper,
			body_middle_d6_transform,
			body_upper_capsule_bottom_transform,
			pelvis_joint_transform,
			thorax_joint_transform,
			-10.0f, 10.0f,
			15.0f,
			15.0f);

		const float arm_forward_angle_deg = 32.5f; // How far an arm can be rotated forward around Y axis
		const float arm_backward_angle_deg = 70.0f; // How far an arm can be rotated backwards around Y axis
		const float arm_d6_swing_y_deg = (arm_forward_angle_deg + arm_backward_angle_deg) / 2.0f;

		// Thorax -> left arm
		const vec3 left_arm_capsule_y_axis = left_arm_capsule_bottom_transform.rotation * vec3(0.0f, 1.0f, 0.0f);
		const trs thorax_to_left_arm_d6_transform = trs(
			left_arm_capsule_bottom_transform.position,
			quat(left_arm_capsule_y_axis, deg2rad(arm_d6_swing_y_deg - arm_backward_angle_deg)) * left_arm_capsule_bottom_transform.rotation,
			left_arm_capsule_bottom_transform.scale);

		create_d6_joint(
			ragdoll,
			arm_constraint,
			body_upper, left_arm,
			thorax_to_left_arm_d6_transform,
			left_arm_capsule_bottom_transform,
			thorax_joint_transform,
			left_arm_joint_transform,
			-55.0f, 55.0f,
			arm_d6_swing_y_deg,
			60.0f);

		// Left arm -> left forearm
		const float forearm_d6_swing_y_deg = 45.0f;
		const vec3 left_forearm_capsule_y_axis = left_forearm_capsule_bottom_transform.rotation * vec3(0.0f, 1.0f, 0.0f);
		const trs left_forearm_d6_transform = trs(
			left_forearm_capsule_bottom_transform.position,
			quat(left_forearm_capsule_y_axis, deg2rad(forearm_d6_swing_y_deg)) * left_forearm_capsule_bottom_transform.rotation,
			left_forearm_capsule_bottom_transform.scale);

		create_d6_joint(
			ragdoll, forearm_constraint,
			left_arm, left_forearm,
			left_forearm_d6_transform,
			left_forearm_capsule_bottom_transform,
			left_arm_joint_transform,
			left_forearm_joint_transform,
			-10.0f, 10.0f,
			forearm_d6_swing_y_deg,
			8.0f);

		// Left forearm -> left hand
		create_d6_joint(
			ragdoll, hand_constraint,
			left_forearm, left_hand,
			left_hand_box_bottom_transform,
			left_hand_box_bottom_transform,
			left_forearm_joint_transform,
			left_hand_joint_transform,
			-8.0f, 8.0f,
			22.0f,
			40.0f);

		// Thorax -> right arm
		const vec3 right_arm_capsule_y_axis = right_arm_capsule_bottom_transform.rotation * vec3(0.0f, 1.0f, 0.0f);
		const trs thorax_to_right_arm_d6_transform = trs(
			right_arm_capsule_bottom_transform.position,
			quat(right_arm_capsule_y_axis, deg2rad(arm_d6_swing_y_deg - arm_backward_angle_deg)) * right_arm_capsule_bottom_transform.rotation,
			right_arm_capsule_bottom_transform.scale);

		create_d6_joint(
			ragdoll, arm_constraint,
			body_upper, right_arm,
			thorax_to_right_arm_d6_transform,
			right_arm_capsule_bottom_transform,
			thorax_joint_transform,
			right_arm_joint_transform,
			-55.0f, 55.0f,
			arm_d6_swing_y_deg,
			60.0f);

		// Right arm -> right forearm
		const vec3 right_forearm_capsule_y_axis = right_forearm_capsule_bottom_transform.rotation * vec3(0.0f, 1.0f, 0.0f);
		const trs right_forearm_d6_transform = trs(
			right_forearm_capsule_bottom_transform.position,
			quat(right_forearm_capsule_y_axis, deg2rad(forearm_d6_swing_y_deg)) * right_forearm_capsule_bottom_transform.rotation,
			right_forearm_capsule_bottom_transform.scale);

		create_d6_joint(
			ragdoll, forearm_constraint,
			right_arm, right_forearm,
			right_forearm_d6_transform,
			right_forearm_capsule_bottom_transform,
			right_arm_joint_transform,
			right_forearm_joint_transform,
			-10.0f, 10.0f,
			forearm_d6_swing_y_deg,
			8.0f);

		// Right forearm -> right hand
		create_d6_joint(
			ragdoll, hand_constraint,
			right_forearm, right_hand,
			right_hand_box_bottom_transform,
			right_hand_box_bottom_transform,
			right_forearm_joint_transform,
			right_hand_joint_transform,
			-8.0f, 8.0f,
			22.0f,
			40.0f);

		// Pelvis -> left up leg
		const float up_leg_back_angle_deg = 75.5f; // How far up leg can be rotated around y axis in backwards direction
		const float up_leg_forward_angle_deg = 25.0f; // How far up leg can be rotated around y axis in forward direction
		const float up_leg_d6_swing_y_deg = (up_leg_forward_angle_deg + up_leg_back_angle_deg) / 2.0f;
		const vec3 left_up_leg_capsule_y_axis = left_up_leg_capsule_bottom_transform.rotation * vec3(0.0f, 1.0f, 0.0f);
		const trs left_up_leg_d6_transform = trs(
			left_up_leg_capsule_bottom_transform.position,
			quat(left_up_leg_capsule_y_axis, deg2rad(up_leg_d6_swing_y_deg - up_leg_back_angle_deg)) * left_up_leg_capsule_bottom_transform.rotation,
			left_up_leg_capsule_bottom_transform.scale);
		create_d6_joint(
			ragdoll, leg_constraint,
			body_lower, left_up_leg,
			left_up_leg_d6_transform,
			left_up_leg_capsule_bottom_transform,
			pelvis_joint_transform,
			left_up_leg_joint_transform,
			-20.0f, 20.0f,
			up_leg_d6_swing_y_deg,
			35.0f);

		// Left up leg -> left leg
		const float leg_d6_swing_y_deg = 45.0f;
		const vec3 left_leg_capsule_y_axis = left_leg_capsule_bottom_transform.rotation * vec3(0.0f, 1.0f, 0.0f);
		const trs left_leg_d6_transform = trs(
			left_leg_capsule_bottom_transform.position,
			quat(left_leg_capsule_y_axis, deg2rad(leg_d6_swing_y_deg)) * left_leg_capsule_bottom_transform.rotation,
			left_leg_capsule_bottom_transform.scale);
		create_d6_joint(
			ragdoll, calf_constraint,
			left_up_leg, left_leg,
			left_leg_d6_transform,
			left_leg_capsule_bottom_transform,
			left_up_leg_joint_transform,
			left_leg_joint_transform,
			-10.0f, 10.0f,
			leg_d6_swing_y_deg,
			5.0f);

		// Left leg -> left foot
		create_d6_joint(
			ragdoll, foot_constraint,
			left_leg, left_foot,
			left_foot_box_bottom_transform,
			left_foot_box_bottom_transform,
			left_leg_joint_transform,
			left_foot_joint_transform,
			-17.0f, 17.0f,
			35.0f,
			22.5f);

		// Pelvis -> right up leg
		const vec3 right_up_leg_capsule_y_axis = right_up_leg_capsule_bottom_transform.rotation * vec3(0.0f, 1.0f, 0.0f);
		const trs right_up_leg_d6_transform = trs(
			right_up_leg_capsule_bottom_transform.position,
			quat(right_up_leg_capsule_y_axis, deg2rad(up_leg_d6_swing_y_deg - up_leg_back_angle_deg)) * right_up_leg_capsule_bottom_transform.rotation,
			right_up_leg_capsule_bottom_transform.scale);
		create_d6_joint(
			ragdoll, leg_constraint,
			body_lower, right_up_leg,
			right_up_leg_d6_transform,
			right_up_leg_capsule_bottom_transform,
			pelvis_joint_transform,
			right_up_leg_joint_transform,
			-20.0f, 20.0f,
			up_leg_d6_swing_y_deg,
			35.0f);

		// Right up leg -> right leg
		const vec3 right_leg_capsule_y_axis = right_leg_capsule_bottom_transform.rotation * vec3(0.0f, 1.0f, 0.0f);
		const trs right_leg_d6_transform = trs(
			right_leg_capsule_bottom_transform.position,
			quat(right_leg_capsule_y_axis, deg2rad(leg_d6_swing_y_deg)) * right_leg_capsule_bottom_transform.rotation,
			right_leg_capsule_bottom_transform.scale);
		create_d6_joint(
			ragdoll, calf_constraint,
			right_up_leg, right_leg,
			right_leg_d6_transform,
			right_leg_capsule_bottom_transform,
			right_up_leg_joint_transform,
			right_leg_joint_transform,
			-10.0f, 10.0f,
			leg_d6_swing_y_deg,
			5.0f);

		// Right leg -> right foot
		create_d6_joint(
			ragdoll, foot_constraint,
			right_leg, right_foot,
			right_foot_box_bottom_transform,
			right_foot_box_bottom_transform,
			right_leg_joint_transform,
			right_foot_joint_transform,
			-17.0f, 17.0f,
			35.0f,
			22.5f);
	}

}