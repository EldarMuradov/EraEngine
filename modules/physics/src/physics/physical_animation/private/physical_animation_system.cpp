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
#include "physics/physical_animation/drive_pose_solver.h"
#include "physics/physical_animation/physical_animation_utils.h"

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

	static DebugVar<bool> enable_always = DebugVar<bool>("physics.physical_animation.enable_always", false);
	static DebugVar<bool> force_drive_simulation = DebugVar<bool>("physics.physical_animation.force_drive_simulation", false);

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

			idle_profile->drive_angular_velocity_modifier = 0.8f;

			idle_profile->type = RagdollProfileType::IDLE;
			idle_profile->head_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
			idle_profile->neck_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
			idle_profile->arm_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
			idle_profile->body_upper_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
			idle_profile->body_middle_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
			idle_profile->forearm_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
			idle_profile->hand_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
			idle_profile->leg_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
			idle_profile->calf_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
			idle_profile->foot_constraint.blend_type = ConstraintBlendType::PURE_PHYSICS;
		}

		{
			running_profile = make_ref<RagdollProfile>();

			running_profile->type = RagdollProfileType::RUNNING;
			running_profile->head_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			running_profile->neck_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			running_profile->arm_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			running_profile->body_upper_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			running_profile->body_middle_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			running_profile->forearm_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			running_profile->hand_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			running_profile->leg_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			running_profile->calf_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			running_profile->foot_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
		}

		{
			sprint_profile = make_ref<RagdollProfile>();

			sprint_profile->type = RagdollProfileType::RUNNING;
			sprint_profile->head_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			sprint_profile->neck_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			sprint_profile->arm_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			sprint_profile->body_upper_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			sprint_profile->body_middle_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			sprint_profile->forearm_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			sprint_profile->hand_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			sprint_profile->leg_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			sprint_profile->calf_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
			sprint_profile->foot_constraint.blend_type = ConstraintBlendType::BLEND_WITH_ANIMATION_POSE;
		}

		pose_sampler = std::make_unique<DrivePoseSampler>();
	}

	void PhysicalAnimationSystem::update(float dt)
	{
		ZoneScopedN("PhysicalAnimationSystem::update");

		process_added_pacs();

		for (auto&& [entity_handle, changed_flag, physical_animation_component] : world->group(components_group<TransformComponent, PhysicalAnimationComponent>).each())
		{
			if (physical_animation_component.simulated.is_changed())
			{
				for (const EntityPtr& limb_ptr : physical_animation_component.limbs)
				{
					Entity limb = limb_ptr.get();
					limb.get_component<PhysicalAnimationLimbComponent>()->simulated = physical_animation_component.simulated;
				}
				physical_animation_component.simulated.sync_changes();
			}
		}

		for (auto&& [entity_handle, changed_flag, physical_animation_limb_component] : world->group(components_group<TransformComponent, PhysicalAnimationLimbComponent>).each())
		{
			if (physical_animation_limb_component.simulated.is_changed())
			{
				Entity limb = world->get_entity(entity_handle);
				limb.get_component<DynamicBodyComponent>()->simulated = physical_animation_limb_component.simulated;
				physical_animation_limb_component.simulated.sync_changes();
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

		update_ragdolls(dt);
	}

	void PhysicalAnimationSystem::update_normal(float dt)
	{
		using namespace animation;

		ZoneScopedN("PhysicalAnimationSystem::update_normal");

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

			const bool is_simulated = physical_animation_component.get_current_state_type() != SimulationStateType::DISABLED;

			if (is_simulated)
			{
				Skeleton* skeleton = skeleton_component.skeleton;
				if (skeleton == nullptr)
				{
					continue;
				}

				SkeletonPose current_animation_pose = animation_component.current_animation_pose;

				// Sample only if simulation enabled.
				pose_sampler->sample_pose(&physical_animation_component, &skeleton_component, current_animation_pose);
			}
		}
	}

	void PhysicalAnimationSystem::update_ragdolls(float dt)
	{
		using namespace animation;

		bool has_any_simulated_ragdolls = false;

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
			physical_animation_component.velocity = root_delta_motion / dt;

			if (is_should_be_simulated) // Update profiles only if simulated.
			{
				update_ragdoll_profiles(&physical_animation_component, root_delta_motion, dt, false);
			}

			// Update states.
			{
				const SimulationStateType desired_state_type = is_should_be_simulated ? SimulationStateType::ENABLED : SimulationStateType::DISABLED;
				physical_animation_component.update_states(dt, desired_state_type);
			}

			const SimulationStateType current_state_type = physical_animation_component.get_current_state_type();
			const bool is_disabled = current_state_type == SimulationStateType::DISABLED;

			if (!is_disabled)
			{
				has_any_simulated_ragdolls = true;

				std::queue<uint32> simulation_limbs_queue;
				simulation_limbs_queue.push(physical_animation_component.root_joint_id);

				trs pelvis_local_transform = SkeletonUtils::get_object_space_joint_transform(current_animation_pose, skeleton, physical_animation_component.root_joint_id, 0);
				pelvis_local_transform.scale = vec3(1.0f);

				physical_animation_component.local_joint_poses_for_target_calculation[physical_animation_component.root_joint_id] = pelvis_local_transform;

				calculate_state_poses(&physical_animation_component, physical_animation_component.simulated_joints.at(physical_animation_component.root_joint_id).get(), pelvis_local_transform, world_transform);

				// Traverse simulation graph once.
				while (!simulation_limbs_queue.empty())
				{
					const uint32 current_id = simulation_limbs_queue.front();
					simulation_limbs_queue.pop();

					for (const uint32 child_id : physical_animation_component.children_map[current_id])
					{
						const trs& parent_local = physical_animation_component.local_joint_poses_for_target_calculation.at(current_id);

						trs limb_animation_pose = current_animation_pose.get_joint_transform(child_id).get_transform();
						limb_animation_pose.scale = vec3(1.0f);

						// Update targets from current animation.
						trs calculated_transform = parent_local * limb_animation_pose;
						calculated_transform.scale = vec3(1.0f);
						physical_animation_component.local_joint_poses_for_target_calculation[child_id] = calculated_transform;

						auto limb_iter = physical_animation_component.simulated_joints.find(child_id);
						if (limb_iter != physical_animation_component.simulated_joints.end())
						{
							const Entity limb = limb_iter->second.get();
							calculate_state_poses(&physical_animation_component, limb, calculated_transform, world_transform);
						}

						simulation_limbs_queue.push(child_id);
					}
				}

				const trs pelvis_world_transform = world_transform * pelvis_local_transform;
				PhysicsUtils::manual_set_physics_transform(physical_animation_component.attachment_body.get(), pelvis_world_transform, true);
			}

			physical_animation_component.prev_fixed_world_transform = world_transform;
		}

		if (has_any_simulated_ragdolls)
		{
			filter_states_by_collisions(dt);

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

				const SimulationStateType current_state_type = physical_animation_component.get_current_state_type();
				const bool is_simulated = current_state_type != SimulationStateType::DISABLED;

				if (is_simulated)
				{
					update_chains_states(&physical_animation_component, dt);
					physical_animation_component.pose_solver->solve_pose(dt);
				}
			}
		}
	}

	void PhysicalAnimationSystem::filter_states_by_collisions(float dt) const
	{
		ref<SimulationEventCallback> event_callback = PhysicsHolder::physics_ref->simulation_event_callback;
		for (const Collision& collision : event_callback->new_collisions)
		{
			Entity first_entity = collision.this_actor->get_entity();
			Entity second_entity = collision.other_actor->get_entity();

			if (first_entity == second_entity)
			{
				continue;
			}

			if (first_entity.get_parent_handle() == second_entity.get_parent_handle())
			{
				continue;
			}

			PhysicalAnimationLimbComponent* first_limb_data_component = first_entity.get_component<PhysicalAnimationLimbComponent>();
			if (first_limb_data_component == nullptr)
			{
				continue;
			}

			PhysicalAnimationLimbComponent* second_limb_data_component = second_entity.get_component<PhysicalAnimationLimbComponent>();
			if (second_limb_data_component == nullptr)
			{
				continue;
			}

			Entity first_ragdoll = first_limb_data_component->ragdoll_ptr.get();
			const PhysicalAnimationComponent* first_simulation_component = first_ragdoll.get_component<PhysicalAnimationComponent>();
			ASSERT(first_simulation_component != nullptr);
			first_simulation_component->pose_solver->force_solve_collided_limb(first_limb_data_component);

			Entity second_ragdoll = second_limb_data_component->ragdoll_ptr.get();
			const PhysicalAnimationComponent* second_simulation_component = second_ragdoll.get_component<PhysicalAnimationComponent>();
			ASSERT(second_simulation_component != nullptr);
			second_simulation_component->pose_solver->force_solve_collided_limb(second_limb_data_component);
		}
	}

	void PhysicalAnimationSystem::update_chains_states(const PhysicalAnimationComponent* physical_animation_component, float dt) const
	{
		bool has_any_simulated_chains = false;

		has_any_simulated_chains |= check_chain(physical_animation_component->left_arm_chain);
		has_any_simulated_chains |= check_chain(physical_animation_component->right_arm_chain);
		has_any_simulated_chains |= check_chain(physical_animation_component->left_leg_chain);
		has_any_simulated_chains |= check_chain(physical_animation_component->right_leg_chain);

		// Reset forse simulation on body chain if not idle.
		if (!physical_animation_component->is_in_idle())
		{
			has_any_simulated_chains = false;
		}

		// If we have at least one simulated chain, we should also simulate the body root chain.
		update_chain(physical_animation_component->body_chain, dt, has_any_simulated_chains);
		update_chain(physical_animation_component->neck_chain, dt, has_any_simulated_chains);

		update_chain(physical_animation_component->left_arm_chain, dt);
		update_chain(physical_animation_component->right_arm_chain, dt);
		update_chain(physical_animation_component->left_leg_chain, dt);
		update_chain(physical_animation_component->right_leg_chain, dt);
	}

	bool PhysicalAnimationSystem::update_chain(const ref<PhysicsLimbChain>& chain, float dt, bool force_simulation) const
	{
		const bool should_update_as_simulated = check_chain(chain, force_simulation);

		for (EntityPtr limb_ptr : chain->connected_limbs)
		{
			Entity limb = limb_ptr.get();

			PhysicalAnimationLimbComponent* limb_data_component = limb.get_component<PhysicalAnimationLimbComponent>();
			ASSERT(limb_data_component != nullptr);

			bool want_be_simulated = should_update_as_simulated;

			ConstraintLimbStateType desired_chain_state = want_be_simulated ? ConstraintLimbStateType::SIMULATION : ConstraintLimbStateType::KINEMATIC;
			if (force_drive_simulation)
			{
				desired_chain_state = ConstraintLimbStateType::SIMULATION;
			}

			limb_data_component->update_states(dt, desired_chain_state);

			if (limb_data_component->was_in_collision && limb_data_component->is_colliding)
			{
				limb_data_component->collision_time = clamp(limb_data_component->collision_time + dt, 0.0f, limb_data_component->max_collision_time);
			}
			else
			{
				limb_data_component->collision_time = max(limb_data_component->collision_time - dt, 0.0f);
			}
			limb_data_component->was_in_collision = limb_data_component->is_colliding;
		}

		return should_update_as_simulated;
	}

	bool PhysicalAnimationSystem::check_chain(const ref<PhysicsLimbChain>& chain, bool force_simulation) const
	{
		const bool should_update_as_simulated = chain->has_any_blocked_limb() || chain->has_any_colliding_limb() || force_simulation;
		return should_update_as_simulated;
	}

	void PhysicalAnimationSystem::calculate_state_poses(const PhysicalAnimationComponent* physical_animation_component, Entity limb, const trs& calculated_target_local_space_pose, const trs& world_space_ragdoll_transform) const
	{
		PhysicalAnimationLimbComponent* limb_data_component = limb.get_component<PhysicalAnimationLimbComponent>();
		ASSERT(limb_data_component != nullptr);

		// Assume that PxRigidDynamic::getGlobalPose() is equal to entitie's world transform.
		limb_data_component->physics_pose = limb.get_component<TransformComponent>()->get_world_transform();

		// Calculate target pose in world space.
		trs world_space_limb_transform = world_space_ragdoll_transform * calculated_target_local_space_pose;
		world_space_limb_transform.scale = vec3(1.0f);
		limb_data_component->target_pose = world_space_limb_transform;

		if(!physical_animation_component->simulated.get())
		{
			limb_data_component->adjusted_pose = limb_data_component->target_pose;
		}

		// Reset at the beggining of the calculation process.
		limb_data_component->is_colliding = false;
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

	vec3 PhysicalAnimationSystem::get_joint_adjustment(const PhysicalAnimationComponent* physical_animation_component, uint32 joint_id) const
	{
		if (joint_id == physical_animation_component->joint_init_ids.pelvis_idx)
		{
			return physical_animation_component->settings.pelvis_joint_adjastment;
		}
		else if (joint_id == physical_animation_component->joint_init_ids.spine_01_idx)
		{
			return physical_animation_component->settings.abdomen_joint_adjastment;
		}
		else if (joint_id == physical_animation_component->joint_init_ids.spine_03_idx)
		{
			return physical_animation_component->settings.thorax_joint_adjastment;
		}
		else if (joint_id == physical_animation_component->joint_init_ids.head_idx)
		{
			return physical_animation_component->settings.head_joint_adjastment;
		}
		else if (joint_id == physical_animation_component->joint_init_ids.neck_idx)
		{
			return physical_animation_component->settings.neck_joint_adjastment;
		}

		return vec3::zero;
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
			physical_animation_component->local_joint_poses_for_target_calculation.reserve(joints_size);
		}
	}

	void PhysicalAnimationSystem::on_pac_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ sync };

		pacs_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}

}
