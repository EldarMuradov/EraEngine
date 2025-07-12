#include "physics/physical_animation/physical_animation_system.h"
#include "physics/core/physics.h"
#include "physics/core/physics_utils.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"
#include "physics/collisions_holder_root_component.h"
#include "physics/physical_animation/physical_animation_builder.h"
#include "physics/scene_queries.h"
#include "physics/shape_component.h"
#include "physics/ragdoll_profile.h"

#include <core/cpu_profiling.h>
#include <core/debug/debug_var.h>
#include <core/log.h>
#include <core/traits.h>

#include <rendering/debug_visualization.h>
#include <rendering/ecs/renderer_holder_root_component.h>

#include <ecs/update_groups.h>

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine::physics
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<PhysicalAnimationSystem>("PhysicalAnimationSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))

			.method("update", &PhysicalAnimationSystem::update)
			(metadata("update_group", update_types::PHYSICS),
			 metadata("After", std::vector<std::string>{"PhysicsSystem::update"}))

			.method("update_normal", &PhysicalAnimationSystem::update_normal)
			(metadata("update_group", update_types::GAMEPLAY_NORMAL));
	}

	static DebugVar<bool> enable_angular_drive = DebugVar<bool>("physics.physical_animation.enable_angular_drive", true);
	static DebugVar<bool> enable_motor_drive = DebugVar<bool>("physics.physical_animation.enable_motor_drive", true);
	static DebugVar<bool> enable_dynamic_raycasts = DebugVar<bool>("physics.physical_animation.enable_dynamic_raycasts", true);
	static DebugVar<bool> enable_always = DebugVar<bool>("physics.physical_animation.enable_always", false);

	static DebugVar<bool> draw_motor_drives = DebugVar<bool>("physics.physical_animation.draw_motor_drives", false);
	static DebugVar<bool> draw_joint_poses = DebugVar<bool>("physics.physical_animation.draw_joint_poses", false);
	static DebugVar<bool> draw_simulation_data = DebugVar<bool>("physics.physical_animation.draw_simulation_data", false);
	static DebugVar<bool> draw_adjusted_positions = DebugVar<bool>("physics.physical_animation.draw_adjusted_positions", false);
	static DebugVar<bool> draw_physics_positions = DebugVar<bool>("physics.physical_animation.draw_physics_positions", false);
	static DebugVar<bool> draw_target_positions = DebugVar<bool>("physics.physical_animation.draw_target_positions", false);

	static DebugVar<float> running_speed = DebugVar<float>("physics.physical_animation.debug_demo.running_speed", 1.0f);
	static DebugVar<float> sprint_speed = DebugVar<float>("physics.physical_animation.debug_demo.sprint_speed", 5.0f);

	PhysicalAnimationSystem::PhysicalAnimationSystem(World* _world)
		: System(_world)
	{
	}

	void PhysicalAnimationSystem::init()
	{
		using namespace animation;
		entt::registry& registry = world->get_registry();
		registry.on_construct<PhysicalAnimationComponent>().connect<&PhysicalAnimationSystem::on_pac_created>(this);

		ragdolls_group = world->group(components_group<TransformComponent, PhysicalAnimationComponent, AnimationComponent, SkeletonComponent>);

		collisions_holder_rc = world->add_root_component<CollisionsHolderRootComponent>();
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();

		{
			idle_profile = make_ref<RagdollProfile>();

			idle_profile->arm_constraint.enable_slerp_drive = true;
			idle_profile->head_constraint.enable_slerp_drive = true;
			idle_profile->leg_constraint.enable_slerp_drive = true;
		}

		{
			running_profile = make_ref<RagdollProfile>();

			running_profile->arm_constraint.enable_slerp_drive = true;
			running_profile->head_constraint.enable_slerp_drive = true;
			running_profile->leg_constraint.enable_slerp_drive = true;

			running_profile->head_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			running_profile->neck_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			running_profile->arm_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			running_profile->body_upper_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			running_profile->body_middle_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			running_profile->forearm_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			running_profile->hand_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			running_profile->leg_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			running_profile->calf_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			running_profile->foot_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
		}

		{
			sprint_profile = make_ref<RagdollProfile>();

			sprint_profile->arm_constraint.enable_slerp_drive = true;
			sprint_profile->head_constraint.enable_slerp_drive = true;
			sprint_profile->leg_constraint.enable_slerp_drive = true;

			sprint_profile->head_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			sprint_profile->neck_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			sprint_profile->arm_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			sprint_profile->body_upper_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			sprint_profile->body_middle_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			sprint_profile->forearm_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			sprint_profile->hand_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			sprint_profile->leg_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			sprint_profile->calf_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
			sprint_profile->foot_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE;
		}
	}

	void PhysicalAnimationSystem::update(float dt)
	{
		process_added_pacs();

		for (auto&& [entity_handle, changed_flag, physical_animation_limb_component] : world->group(components_group<ObservableMemberChangedFlagComponent, PhysicalAnimationLimbComponent>).each())
		{
			if (physical_animation_limb_component.simulated.is_changed())
			{
				Entity limb = world->get_entity(entity_handle);
				limb.get_component<DynamicBodyComponent>()->simulated = physical_animation_limb_component.simulated;
			}
		}

		for (auto&& [entity_handle, changed_flag, physical_animation_component] : world->group(components_group<ObservableMemberChangedFlagComponent, PhysicalAnimationComponent>).each())
		{
			if (physical_animation_component.simulated.is_changed())
			{
				for (const EntityPtr& limb_ptr : physical_animation_component.limbs)
				{
					Entity limb = limb_ptr.get();
					limb.get_component<PhysicalAnimationLimbComponent>()->simulated = physical_animation_component.simulated;
				}
			}
		}

		for (auto&& [entity_handle, transform_component, physical_animation_component, animm, skeleton] : ragdolls_group.each())
		{
			if (physical_animation_component.loaded)
			{
				continue;
			}
			bool is_ready_for_simulation = true;
			for (const EntityPtr& limb_ptr : physical_animation_component.limbs)
			{
				Entity limb = limb_ptr.get();

				ShapeComponent* shape_component = ShapeUtils::get_shape_component(limb);
				DynamicBodyComponent* body_component = limb.get_component<DynamicBodyComponent>();

				if (body_component != nullptr && shape_component != nullptr)
				{
					is_ready_for_simulation &= body_component->get_rigid_actor() != nullptr;
					is_ready_for_simulation &= shape_component->get_shape() != nullptr;
					if (body_component->get_rigid_actor() != nullptr)
					{
						is_ready_for_simulation &= body_component->get_rigid_actor()->userData != nullptr;
					}
					if (shape_component->get_shape() != nullptr)
					{
						is_ready_for_simulation &= shape_component->get_shape()->userData != nullptr;
					}
				}
				else
				{
					is_ready_for_simulation = false;
					break;
				}

				if (!is_ready_for_simulation)
				{
					break;
				}
			}

			physical_animation_component.loaded = is_ready_for_simulation;
		}

		update_limb_states_and_profiles(dt);
	}

	void PhysicalAnimationSystem::update_normal(float dt)
	{
		using namespace animation;

		for (auto&& [entity_handle,
			transform_component,
			physical_animation_component,
			animation_component, 
			skeleton_component] : ragdolls_group.each())
		{
			if (!physical_animation_component.simulated)
			{
				continue;
			}

			Entity ragdoll = world->get_entity(entity_handle);

			Skeleton* skeleton = skeleton_component.skeleton;
			if (skeleton == nullptr)
			{
				continue;
			}

			SkeletonPose current_animation_pose = animation_component.current_animation_pose;

			const uint32 root_id = physical_animation_component.root_joint_id;
			physical_animation_component.local_joint_poses[root_id] = SkeletonUtils::get_object_space_joint_transform(current_animation_pose, skeleton, root_id);

			const trs pelvis_world_transform = transform_component.get_world_transform() *
				SkeletonUtils::get_object_space_joint_transform(current_animation_pose, skeleton, physical_animation_component.joint_init_ids.attachment_idx);
			PhysicsUtils::manual_set_physics_transform(physical_animation_component.attachment_body.get(), pelvis_world_transform, true);

			const trs inverse_ragdoll_world_transform = invert(physical_animation_component.prev_fixed_world_transform);

			std::queue<uint32> q;
			q.push(root_id);

			while (!q.empty())
			{
				uint32 current_id = q.front();
				q.pop();

				for (uint32 child_id : physical_animation_component.children_map[current_id])
				{
					const trs& parent_local = physical_animation_component.local_joint_poses[current_id];
					trs inverse_parent_local = invert(parent_local);
					inverse_parent_local.rotation = normalize(inverse_parent_local.rotation);

					const trs limb_animation_pose = SkeletonUtils::get_object_space_joint_transform(current_animation_pose, skeleton, child_id);

					auto limb_iter = physical_animation_component.simulated_joints.find(child_id);
					if (limb_iter == physical_animation_component.simulated_joints.end())
					{
						physical_animation_component.local_joint_poses[child_id] = limb_animation_pose;
					}
					else
					{
						Entity limb = limb_iter->second.get();
						PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();
						const trs& limb_physics_pose = limb.get_component<TransformComponent>()->get_world_transform();

						const trs limb_pose = inverse_ragdoll_world_transform * limb_physics_pose;

						trs new_transform = trs::identity;
						if (limb_component->blend_type == ConstraintBlendType::BLEND_WITH_ADJUSTED_POSE)
						{
							const vec3 mid_translation = lerp(limb_component->adjusted_pose.position,
								limb_component->physics_pose.position,
								physical_animation_component.blend_factor);

							const quat mid_rotation = slerp(limb_component->adjusted_pose.rotation,
								limb_component->physics_pose.rotation,
								physical_animation_component.blend_factor);

							new_transform = inverse_parent_local * inverse_ragdoll_world_transform * trs(mid_translation, mid_rotation, vec3(1.0f));
						}
						else if (limb_component->blend_type == ConstraintBlendType::BLEND_WITH_PREV_POSE)
						{
							new_transform = inverse_parent_local * limb_pose;

							// Smooth blend with prev pose.
							if (!fuzzy_equals(limb_component->prev_limb_local_rotation, new_transform.rotation))
							{
								new_transform.rotation = slerp(limb_component->prev_limb_local_rotation,
									new_transform.rotation,
									physical_animation_component.blend_factor);
							}
						}
						else
						{
							// No blend at all.
							new_transform = inverse_parent_local * limb_pose;
						}

						// Blend with animation step.
						if (!fuzzy_equals(physical_animation_component.blend_weight, 1.0f))
						{
							const trs animation_blend_pose = limb_animation_pose * inverse_parent_local;
							new_transform.position = lerp(animation_blend_pose.position,
								new_transform.position,
								physical_animation_component.blend_weight);

							new_transform.rotation = slerp(animation_blend_pose.rotation,
								new_transform.rotation,
								physical_animation_component.blend_weight);
						}

						new_transform.rotation = normalize(new_transform.rotation);

						skeleton->set_joint_rotation(new_transform.rotation, child_id);
						current_animation_pose.set_joint_rotation(new_transform.rotation, child_id);

						limb_component->prev_limb_local_position = new_transform.position;
						limb_component->prev_limb_local_rotation = new_transform.rotation;

						trs new_local_child_transform = parent_local * new_transform;
						new_local_child_transform.position = new_local_child_transform.position;
						new_local_child_transform.rotation = normalize(new_local_child_transform.rotation);

						physical_animation_component.local_joint_poses[child_id] = new_local_child_transform;
					}

					q.push(child_id);
				}
			}
		}
	}

	void PhysicalAnimationSystem::update_limb_states_and_profiles(float dt)
	{
		using namespace animation;

		for (auto&& [entity_handle,
			transform_component,
			physical_animation_component,
			animation_component,
			skeleton_component] : ragdolls_group.each())
		{
			if (!physical_animation_component.loaded)
			{
				continue;
			}

			const Skeleton* skeleton = skeleton_component.skeleton;
			if (skeleton == nullptr)
			{
				ASSERT(false);
				continue;
			}

			if (animation_component.animation == nullptr)
			{
				continue;
			}

			const SkeletonPose& current_animation_pose = animation_component.current_animation_pose;

			Entity simulated_body = world->get_entity(entity_handle);

			const trs& world_transform = simulated_body.get_component<TransformComponent>()->get_world_transform();

			const bool is_should_be_simulated = should_be_simulated(simulated_body);

			const vec3 root_delta_motion = world_transform.position - physical_animation_component.prev_fixed_world_transform.position;
			const vec3 raw_root_acceleration = root_delta_motion / (dt * dt);

			if (is_should_be_simulated) // Update profiles only if simulated.
			{
				update_ragdoll_profiles(&physical_animation_component, root_delta_motion, dt, false);
			}

			// Update states.
			{
				const ConstraintStateType desired_state_type = is_should_be_simulated ? ConstraintStateType::ENABLED : ConstraintStateType::DISABLED;
				physical_animation_component.update_states(dt, desired_state_type);
			}

			const ConstraintStateType current_state_type = physical_animation_component.get_current_state_type();

			for (const EntityPtr& limb_ptr : physical_animation_component.limbs)
			{
				Entity limb = limb_ptr.get();

				const TransformComponent* limb_transform = limb.get_component<TransformComponent>();
				const trs& current_limb_pose = limb_transform->get_world_transform(); // Assume that PxRigidDynamic::getGlobalPose() is equal to entitie's world transform.

				const bool is_limb_disabled = current_state_type == ConstraintStateType::DISABLED;

				PhysicalAnimationLimbComponent* limb_component = limb.get_component<PhysicalAnimationLimbComponent>();

				// Update targets from current animation.
				{
					trs joint_anim_transform = SkeletonUtils::get_object_space_joint_transform(current_animation_pose, skeleton, limb_component->joint_id);

					limb_component->target_pose = world_transform * joint_anim_transform; // Calculate target pose in world space.
					limb_component->target_pose.scale = vec3(1.0f);
					// So if we skip sync or limb is disabled - just sync all poses to target.
					if (is_limb_disabled)
					{
						limb_component->adjusted_pose = limb_component->target_pose;
						limb_component->physics_pose = limb_component->target_pose;
					}
					else
					{
						// Sync physics world pose to state.
						limb_component->physics_pose = current_limb_pose;
					}
				}

				if (!is_limb_disabled) // Don't apply any motors for ragdoll.
				{
					// Update targets, states and collisions.
					update_targets_and_collisions(limb, limb_component, &physical_animation_component, dt);

					// Apply all drives to limb.
					apply_drives_to_limb(limb, limb_component, &physical_animation_component, raw_root_acceleration, dt);
				}

				if (!is_should_be_simulated)
				{
					continue;
				}

				if (draw_adjusted_positions)
				{
					renderWireSphere(limb_component->adjusted_pose.position,
						0.07f,
						vec4(0.0f, 0.0f, 1.0f, 1.0f),
						renderer_holder_rc->ldrRenderPass);
				}
				if (draw_target_positions)
				{
					renderWireSphere(limb_component->target_pose.position,
						0.07f,
						vec4(0.0f, 1.0f, 0.0f, 1.0f),
						renderer_holder_rc->ldrRenderPass);
				}
				if (draw_physics_positions)
				{
					renderWireSphere(limb_component->physics_pose.position,
						0.07f,
						vec4(1.0f, 0.0f, 0.0f, 1.0f),
						renderer_holder_rc->ldrRenderPass);
				}
			}
			physical_animation_component.prev_fixed_world_transform = world_transform;
		}
	}

	void PhysicalAnimationSystem::update_ragdoll_profiles(PhysicalAnimationComponent* physical_animation_component,
		const vec3& raw_root_delta_position,
		float dt,
		bool force_reload) const
	{
		const vec3 velocity = (raw_root_delta_position) / dt;
		const float velocity_magnitude = length(velocity);
		if (velocity_magnitude >= running_speed && velocity_magnitude <= sprint_speed)
		{
			const bool update_result = physical_animation_component->try_to_apply_ragdoll_profile(running_profile, force_reload);
			if (update_result)
			{
				LOG_MESSAGE("Physics> Ragdoll profile updated to RUNNING");
			}
		}
		else if (velocity_magnitude >= sprint_speed)
		{
			const bool update_result = physical_animation_component->try_to_apply_ragdoll_profile(sprint_profile, force_reload);
			if (update_result)
			{
				LOG_MESSAGE("Physics> Ragdoll profile updated to SPRINT");
			}
		}
		else
		{
			const bool update_result = physical_animation_component->try_to_apply_ragdoll_profile(idle_profile, force_reload);
			if (update_result)
			{
				LOG_MESSAGE("Physics> Ragdoll profile updated to IDLE");
			}
		}
	}

	bool PhysicalAnimationSystem::should_be_simulated(Entity ragdoll) const
	{
		return enable_always;
	}

	void PhysicalAnimationSystem::update_targets_and_collisions(Entity limb,
		PhysicalAnimationLimbComponent* limb_component,
		PhysicalAnimationComponent* physical_animation_component,
		float dt) const
	{
		const vec3 to_target = limb_component->target_pose.position - limb_component->physics_pose.position;
		const float distance = length(to_target);

		// If we are too close to target_pose - we already arrived and there is no need to do raycast check to target pose.
		if (distance > 0.01f && enable_dynamic_raycasts)
		{
			const vec3 direction = to_target / distance;

			// We can collide not only with players. But with other obstacles too. But now it is not used because it works only in physics contest for now.
			SceneQueryFilter raycast_filter = SceneQueryFilter::construct_from_type(world, static_cast<uint32>(CollisionType::RAGDOLL));

			// We also want to skip ignore list in raycast. Assume that the animation doesn't go away on its own.
			for (const EntityPtr& limb_ptr : physical_animation_component->limbs)
			{
				raycast_filter.ignore_list.emplace_back(limb_ptr);
			}
			raycast_filter.ignore_list.emplace_back(limb);

			RaycastQuery::Params params;
			params.query_ray.origin = limb_component->physics_pose.position;
			params.query_ray.direction = direction;
			params.distance = distance + 0.1f; // We need some offset to predict future collisiom.
			params.filter_flags = SceneQueryFilterFlag::STATICS | SceneQueryFilterFlag::DYNAMICS | SceneQueryFilterFlag::PREFILTER;
			params.filter_callback = &raycast_filter;

			SceneQueryPositionedHit hit;
			if (RaycastQuery::closest(world, params, &hit))
			{
				limb_component->is_blocked = true;
				// Smooth go from 0 to 1.
				limb_component->blocked_blend_factor = min(limb_component->blocked_blend_factor + 2.0f * dt, 1.0f);

				// We need to force limit adjusted_pose by physics raycast so that the next force step doesn't try to escape the obstacle.
				const float safe_distance = hit.distance * 0.8f;
				limb_component->adjusted_pose.position = limb_component->physics_pose.position + direction * safe_distance;
				// We don't want the full blend. It will smooth go down from target_pose.rotation to slerp(target_pose.rotation, physics_pose.rotation, ...).
				limb_component->adjusted_pose.rotation = slerp(limb_component->target_pose.rotation,
					limb_component->physics_pose.rotation,
					1.0f - limb_component->blocked_blend_factor * 0.7f);
			}
			else
			{
				limb_component->is_blocked = false;
				// Smooth go from 1 to 0.
				limb_component->blocked_blend_factor = max(limb_component->blocked_blend_factor - 3.0f * dt, 0.0f);

				// Event if blend_factor is close to 0 we need to consider physics_pose.
				limb_component->adjusted_pose.position = lerp(limb_component->target_pose.position,
					limb_component->physics_pose.position,
					max(limb_component->blocked_blend_factor, physical_animation_component->target_position_blend_factor));

				limb_component->adjusted_pose.rotation = slerp(limb_component->target_pose.rotation,
					limb_component->physics_pose.rotation,
					max(limb_component->blocked_blend_factor, physical_animation_component->target_rotation_blend_factor));
			}
		}
		else
		{
			limb_component->is_blocked = false;
			limb_component->blocked_blend_factor = 0.0f; // Reset blend factor.

			limb_component->adjusted_pose.position = lerp(limb_component->target_pose.position,
				limb_component->physics_pose.position,
				physical_animation_component->target_position_blend_factor);

			limb_component->adjusted_pose.rotation = slerp(limb_component->target_pose.rotation,
				limb_component->physics_pose.rotation,
				physical_animation_component->target_rotation_blend_factor);
		}
	}

	void PhysicalAnimationSystem::apply_drives_to_limb(Entity limb,
		PhysicalAnimationLimbComponent* limb_component,
		const PhysicalAnimationComponent* physical_animation_component,
		const vec3& raw_root_acceleration,
		float dt) const
	{
		DynamicBodyComponent* dynamic_body = limb.get_component<DynamicBodyComponent>();
		ASSERT(dynamic_body != nullptr);

		ref<RagdollProfile> profile = physical_animation_component->get_ragdoll_profile();
		ASSERT(profile != nullptr);

		D6JointComponent* parent_joint_component = dynamic_cast<D6JointComponent*>(limb_component->parent_joint_component.get_for_write());

		if (parent_joint_component == nullptr)
		{
			return;
		}

		Entity parent_limb = parent_joint_component->get_first_entity_ptr().get();

		const bool is_leg_limb = is_low_limb(limb_component->type);

		// Keyframe controller stage.
		{
			const vec3 delta_position = limb_component->adjusted_pose.position - limb_component->physics_pose.position;

			// Partial velocity drive.
			vec3 desired_velocity = delta_position / dt;
			const float desired_velocity_magnitude = length(desired_velocity);
			if (desired_velocity_magnitude > profile->partial_velocity_drive_limit)
			{
				desired_velocity = desired_velocity * (profile->partial_velocity_drive_limit / desired_velocity_magnitude);
			}
			vec3 drive_linear_velocity = lerp(dynamic_body->linear_velocity, desired_velocity, profile->partial_velocity_drive * limb_component->drive_velocity_modifier);

			// Partial root acceleration drive.
			vec3 acceleration = raw_root_acceleration;
			const float acceleration_magnitude = length(acceleration);
			if (acceleration_magnitude > profile->acceleration_limit)
			{
				acceleration = acceleration * (profile->acceleration_limit / acceleration_magnitude);
			}
			drive_linear_velocity += acceleration * (is_leg_limb ? profile->legs_acceleration_gain : profile->acceleration_gain);

			dynamic_body->linear_velocity = drive_linear_velocity;
		}

		const trs& joint_local_parent = parent_joint_component->get_first_local_frame();
		const trs& joint_local_child = parent_joint_component->get_second_local_frame();

		const PhysicalAnimationLimbComponent* parent_limb_component = parent_limb.get_component<PhysicalAnimationLimbComponent>();

		// Motors drive for limb.
		if (enable_motor_drive)
		{
			const trs physics_constraint_pose = limb_component->physics_pose * joint_local_child;
			const trs adjusted_constraint_pose = limb_component->adjusted_pose * joint_local_child;

			trs delta_transform = (adjusted_constraint_pose * invert(physics_constraint_pose));
			if (delta_transform.rotation.w < 0.0f)
			{
				delta_transform.rotation = -delta_transform.rotation;
			}
			delta_transform.rotation = normalize(delta_transform.rotation);
			parent_joint_component->drive_transform = delta_transform;

			const vec3 angular_drive_velocity = calculate_delta_rotation_time_derivative(delta_transform.rotation, dt) * profile->drive_angular_velocity_modifier;
			parent_joint_component->angular_drive_velocity = angular_drive_velocity;

			if (draw_motor_drives)
			{
				const float axis_length = 0.2f;

				const trs drive_result_pose = (parent_limb_component->physics_pose * delta_transform * joint_local_parent);

				renderLine(drive_result_pose.position,
					drive_result_pose.position + drive_result_pose.rotation * vec3(axis_length, 0.0f, 0.0f),
					vec4(1.0f, 0.0f, 0.0f, 1.0f),
					renderer_holder_rc->ldrRenderPass);

				renderLine(drive_result_pose.position,
					drive_result_pose.position + drive_result_pose.rotation * vec3(0.0f, axis_length, 0.0f),
					vec4(0.0f, 1.0f, 0.0f, 1.0f),
					renderer_holder_rc->ldrRenderPass);

				renderLine(drive_result_pose.position,
					drive_result_pose.position + drive_result_pose.rotation * vec3(0.0f, 0.0f, axis_length),
					vec4(0.0f, 0.0f, 1.0f, 1.0f),
					renderer_holder_rc->ldrRenderPass);
			}
		}
		else
		{
			parent_joint_component->drive_transform = trs::identity;
			parent_joint_component->angular_drive_velocity = vec3::zero;
		}

		// Angular velocity drives for rotation stabilization.
		if (enable_angular_drive)
		{
			quat delta_rotation = normalize(limb_component->adjusted_pose.rotation * conjugate(limb_component->physics_pose.rotation));
			if (delta_rotation.w < 0.0f)
			{
				delta_rotation = -delta_rotation;
			}

			const float angular_velocity_modifier = is_leg_limb ? profile->legs_blend_torque_stiffness_modifier : profile->blend_torque_stiffness_modifier;
			const vec3 desired_angular_velocity = calculate_delta_rotation_time_derivative(delta_rotation, dt) * angular_velocity_modifier;

			dynamic_body->angular_velocity = desired_angular_velocity;

			if (draw_joint_poses)
			{
				const trs parent_local_pose = parent_limb_component->physics_pose * joint_local_parent;

				renderLine(parent_local_pose.position,
					parent_local_pose.position + parent_local_pose.rotation * vec3(1.0f, 0.0f, 0.0f),
					vec4(1.0f, 0.0f, 1.0f, 1.0f),
					renderer_holder_rc->ldrRenderPass);

			}
		}
	}

	vec3 PhysicalAnimationSystem::calculate_delta_rotation_time_derivative(const quat& delta_rotation, float dt) const
	{
		float delta_angle = 0.0f;
		vec3 delta_axis = vec3::zero;
		get_axis_rotation(delta_rotation, delta_axis, delta_angle);

		const vec3 desired_angular_velocity = normalize(delta_axis) * delta_angle / dt;
		return desired_angular_velocity;
	}

	bool PhysicalAnimationSystem::is_low_limb(PhysicalAnimationLimbComponent::Type type) const
	{
		return type == PhysicalAnimationLimbComponent::Type::LEG ||
			type == PhysicalAnimationLimbComponent::Type::CALF ||
			type == PhysicalAnimationLimbComponent::Type::FOOT;
	}

	void PhysicalAnimationSystem::process_added_pacs()
	{
		using namespace physx;
		using namespace animation;

		ScopedSpinLock _lock{ sync };

		for (Entity::Handle entity_handle : std::exchange(pacs_to_init, {}))
		{
			Entity entity = world->get_entity(entity_handle);

			const SkeletonComponent* skeleton_component = entity.get_component<SkeletonComponent>();

			const Skeleton* skeleton = skeleton_component->skeleton;
			if (skeleton == nullptr)
			{
				continue;
			}

			PhysicalAnimationComponent* physical_animation_component = entity.get_component<PhysicalAnimationComponent>();
			physical_animation_component->current_profile = idle_profile;

			PhysicalAnimationBuilderUtils::build_simulated_body(entity);

			for (const EntityPtr& limb_ptr : physical_animation_component->limbs)
			{
				const PhysicalAnimationLimbComponent* limb_component = limb_ptr.get().get_component<PhysicalAnimationLimbComponent>();
				physical_animation_component->simulated_joints.emplace(limb_component->joint_id, limb_ptr);
			}

			physical_animation_component->root_joint_id = physical_animation_component->joint_init_ids.attachment_idx;

			const size_t joints_size = skeleton->joints.size();

			for (uint32 joint = physical_animation_component->root_joint_id; joint < joints_size; ++joint)
			{
				uint32 parent_id = skeleton->joints[joint].parent_id;
				physical_animation_component->children_map[parent_id].push_back(joint);
			}

			physical_animation_component->local_joint_poses.reserve(joints_size);
		}
	}

	void PhysicalAnimationSystem::on_pac_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ sync };

		pacs_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}

}
