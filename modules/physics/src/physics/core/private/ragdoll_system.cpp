#include "physics/core/ragdoll_system.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"
#include "physics/ragdoll_builder.h"
#include "physics/shape_component.h"

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

		registration::class_<RagdollSystem>("RagdollSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("physics")))

			.method("update", &RagdollSystem::update)
			(metadata("update_group", update_types::PHYSICS),
			 metadata("After", std::vector<std::string>{"PhysicsSystem::update"}))

			.method("update_normal", &RagdollSystem::update_normal)
			(metadata("update_group", update_types::GAMEPLAY_NORMAL));
	}

	RagdollSystem::RagdollSystem(World* _world)
		: System(_world)
	{
	}

	void RagdollSystem::init()
	{
		entt::registry& registry = world->get_registry();
		registry.on_construct<RagdollComponent>().connect<&RagdollSystem::on_ragdoll_created>(this);

		ragdolls_group = world->group(components_group<TransformComponent, RagdollComponent, animation::SkeletonComponent>);
	}

	void RagdollSystem::update(float dt)
	{
		ZoneScopedN("RagdollSystem::update");

		process_added_ragdolls();

		for (auto [entity_handle, changed_flag, ragdoll_component] : world->group(components_group<TransformComponent, RagdollComponent>).each())
		{
			if (ragdoll_component.simulated.is_changed())
			{
				for (const EntityPtr& limb_ptr : ragdoll_component.limbs)
				{
					Entity limb = limb_ptr.get();
					limb.get_component<RagdollLimbComponent>()->simulated = ragdoll_component.simulated;
				}
				ragdoll_component.simulated.sync_changes();
			}
		}

		for (auto [entity_handle, changed_flag, ragdoll_limb_component] : world->group(components_group<TransformComponent, RagdollLimbComponent>).each())
		{
			if (ragdoll_limb_component.simulated.is_changed())
			{
				Entity limb = world->get_entity(entity_handle);
				limb.get_component<DynamicBodyComponent>()->simulated = ragdoll_limb_component.simulated;
				ragdoll_limb_component.simulated.sync_changes();
			}
		}

		for (auto&& [entity_handle, transform_component, ragdoll_component, skeleton_component] : ragdolls_group.each())
		{
			if (ragdoll_component.loaded)
			{
				continue;
			}
			bool is_ready_for_simulation = true;
			for (const EntityPtr& limb_ptr : ragdoll_component.limbs)
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

			ragdoll_component.loaded = is_ready_for_simulation;
		}
	}

	void RagdollSystem::update_normal(float dt)
	{
		using namespace animation;

		ZoneScopedN("RagdollSystem::update_normal");

		for (auto&& [entity_handle, transform_component, ragdoll_component, skeleton_component] : ragdolls_group.each())
		{
			if (!ragdoll_component.simulated || !ragdoll_component.loaded)
			{
				continue;
			}

			Entity ragdoll = world->get_entity(entity_handle);

			Skeleton* skeleton = skeleton_component.skeleton;
			if (skeleton == nullptr)
			{
				continue;
			}
			
			{
				const uint32 root_id = 0;
				ragdoll_component.local_joint_poses[root_id] = SkeletonUtils::get_object_space_joint_transform(skeleton, root_id);

				const trs inverse_ragdoll_world_transform = invert(transform_component.transform);

				std::queue<uint32> q;
				q.push(root_id);

				while (!q.empty())
				{
					uint32 current_id = q.front();
					q.pop();

					for (uint32 child_id : ragdoll_component.children_map[current_id])
					{
						const trs& parent_local = ragdoll_component.local_joint_poses[current_id];
						trs inverse_parent_local = invert(parent_local);
						inverse_parent_local.rotation = normalize(inverse_parent_local.rotation);

						trs limb_animation_pose = parent_local * skeleton->get_joint_transform(child_id);
						limb_animation_pose.scale = vec3(1.0f);

						auto limb_iter = ragdoll_component.simulated_joints.find(child_id);
						if (limb_iter == ragdoll_component.simulated_joints.end())
						{
							ragdoll_component.local_joint_poses[child_id] = limb_animation_pose;
						}
						else
						{
							Entity limb = limb_iter->second.get();
							RagdollLimbComponent* ragdoll_limb = limb.get_component<RagdollLimbComponent>();
							const trs& limb_physics_pose = limb.get_component<TransformComponent>()->get_world_transform();

							const trs limb_pose = inverse_ragdoll_world_transform * limb_physics_pose;

							trs new_transform = inverse_parent_local * limb_pose;

							if (!fuzzy_equals(ragdoll_limb->prev_limb_local_rotation, new_transform.rotation))
							{
								new_transform.rotation = slerp(ragdoll_limb->prev_limb_local_rotation,
															   new_transform.rotation,
															   ragdoll_component.blend_factor);
							}

							if (!fuzzy_equals(ragdoll_limb->prev_limb_local_position, new_transform.position))
							{
								new_transform.position = lerp(ragdoll_limb->prev_limb_local_position,
															  new_transform.position,
															  ragdoll_component.blend_factor);
							}

							new_transform.rotation = normalize(new_transform.rotation);

							skeleton->set_joint_transform(new_transform, child_id);

							ragdoll_limb->prev_limb_local_position = new_transform.position;
							ragdoll_limb->prev_limb_local_rotation = new_transform.rotation;

							trs new_local_child_transform = parent_local * new_transform;
							new_local_child_transform.position = new_local_child_transform.position;
							new_local_child_transform.rotation = normalize(new_local_child_transform.rotation);

							ragdoll_component.local_joint_poses[child_id] = new_local_child_transform;
						}

						q.push(child_id);
					}
				}
			}
		}
	}

	void RagdollSystem::process_added_ragdolls()
	{
		using namespace physx;
		using namespace animation;

		ScopedSpinLock _lock{ sync };

		for (Entity::Handle entity_handle : std::exchange(ragdolls_to_init, {}))
		{
			Entity entity = world->get_entity(entity_handle);

			const SkeletonComponent* skeleton_component = entity.get_component<SkeletonComponent>();

			const Skeleton* skeleton = skeleton_component->skeleton;
			if (skeleton == nullptr)
			{
				continue;
			}

			RagdollBuilderUtils::build_simulated_body(entity);

			RagdollComponent* ragdoll_component = entity.get_component<RagdollComponent>();

			for (const EntityPtr& limb_ptr : ragdoll_component->limbs)
			{
				const RagdollLimbComponent* limb_component = limb_ptr.get().get_component<RagdollLimbComponent>();
				ragdoll_component->simulated_joints.emplace(limb_component->joint_id, limb_ptr);
			}

			ragdoll_component->root_joint_id = ragdoll_component->joint_init_ids.attachment_idx;

			const size_t joints_size = skeleton->joints.size();

			for (uint32 joint = ragdoll_component->root_joint_id; joint < joints_size; ++joint)
			{
				uint32 parent_id = skeleton->joints[joint].parent_id;
				ragdoll_component->children_map[parent_id].push_back(joint);
			}

			ragdoll_component->local_joint_poses.reserve(joints_size);
		}
	}

	void RagdollSystem::on_ragdoll_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ sync };

		ragdolls_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}

}
