#include "physics/core/ragdoll_system.h"
#include "physics/core/physics.h"
#include "physics/body_component.h"
#include "physics/joint.h"
#include "physics/core/physics_utils.h"
#include "physics/shape_utils.h"
#include "physics/ragdoll_component.h"
#include "physics/ragdoll_builder.h"

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
	}

	void RagdollSystem::update(float dt)
	{
		process_added_ragdolls();

		for (auto [entity_handle, changed_flag, ragdoll_limb_component] : world->group(components_group<ObservableMemberChangedFlagComponent, RagdollLimbComponent>).each())
		{
			if (ragdoll_limb_component.simulated.is_changed())
			{
				Entity limb = world->get_entity(entity_handle);
				limb.get_component<DynamicBodyComponent>().simulated = ragdoll_limb_component.simulated;
			}
		}

		for (auto [entity_handle, changed_flag, ragdoll_component] : world->group(components_group<ObservableMemberChangedFlagComponent, RagdollComponent>).each())
		{
			if (ragdoll_component.simulated.is_changed())
			{
				for (const EntityPtr& limb_ptr : ragdoll_component.limbs)
				{
					Entity limb = limb_ptr.get();
					limb.get_component<RagdollLimbComponent>().simulated = ragdoll_component.simulated;
				}
			}
		}
	}

	void RagdollSystem::update_normal(float dt)
	{
		using namespace animation;

		for (auto [entity_handle, transform_component, ragdoll_component] : world->group(components_group<TransformComponent, RagdollComponent>).each())
		{
			if (!ragdoll_component.simulated)
			{
				continue;
			}

			Entity ragdoll = world->get_entity(entity_handle);
			SkeletonComponent& skeleton_component = ragdoll.get_component<SkeletonComponent>();

			Skeleton* skeleton = skeleton_component.skeleton;
			if (skeleton == nullptr)
			{
				continue;
			}

			std::unordered_map<uint32, EntityPtr> simulated_joints;

			for (const EntityPtr& limb_ptr : ragdoll_component.limbs)
			{
				simulated_joints.emplace(limb_ptr.get().get_component<RagdollLimbComponent>().joint_id, limb_ptr);
			}

			{
				std::unordered_map<uint32, trs> new_local_poses;
				std::unordered_map<uint32, std::vector<uint32>> children_map;

				uint32 root_id = ragdoll_component.joint_init_ids.root_idx;
				for (uint32 joint = root_id; joint < skeleton->joints.size(); ++joint)
				{
					uint32 parent_id = skeleton->joints[joint].parent_id;
					children_map[parent_id].push_back(joint);
				}

				new_local_poses[root_id] = SkeletonUtils::get_object_space_joint_transform(skeleton, root_id);

				const trs inverse_ragdoll_world_transform = invert(transform_component.transform);

				std::queue<uint32> q;
				q.push(root_id);

				while (!q.empty())
				{
					uint32 current_id = q.front();
					q.pop();

					for (uint32 child_id : children_map[current_id])
					{
						const trs& parent_local = new_local_poses[current_id];
						trs inverse_parent_local = invert(parent_local);
						inverse_parent_local.rotation = normalize(inverse_parent_local.rotation);

						const trs limb_animation_pose = SkeletonUtils::get_object_space_joint_transform(skeleton, child_id);

						auto limb_iter = simulated_joints.find(child_id);
						if (limb_iter == simulated_joints.end())
						{
							new_local_poses[child_id] = limb_animation_pose;
						}
						else
						{
							Entity limb = limb_iter->second.get();
							RagdollLimbComponent& ragdoll_limb = limb.get_component<RagdollLimbComponent>();
							const trs limb_physics_pose = limb.get_component<TransformComponent>().transform;

							const trs limb_pose = inverse_ragdoll_world_transform * limb_physics_pose;

							trs new_transform = inverse_parent_local * limb_pose;

							if (!fuzzy_equals(ragdoll_limb.prev_limb_local_rotation, new_transform.rotation))
							{
								new_transform.rotation = slerp(ragdoll_limb.prev_limb_local_rotation,
															   new_transform.rotation,
															   ragdoll_component.blend_factor);
							}

							if (!fuzzy_equals(ragdoll_limb.prev_limb_local_position, new_transform.position))
							{
								new_transform.position = lerp(ragdoll_limb.prev_limb_local_position,
															  new_transform.position,
															  ragdoll_component.blend_factor);
							}

							new_transform.rotation = normalize(new_transform.rotation);

							skeleton->set_joint_transform(new_transform, child_id);

							ragdoll_limb.prev_limb_local_position = new_transform.position;
							ragdoll_limb.prev_limb_local_rotation = new_transform.rotation;

							trs new_local_child_transform = parent_local * new_transform;
							new_local_child_transform.position = new_local_child_transform.position;
							new_local_child_transform.rotation = normalize(new_local_child_transform.rotation);

							new_local_poses[child_id] = new_local_child_transform;
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

		ScopedSpinLock _lock{ sync };

		for (Entity::Handle entity_handle : std::exchange(ragdolls_to_init, {}))
		{
			Entity entity = world->get_entity(entity_handle);
			RagdollBuilderUtils::build_simulated_body(entity);
		}
	}

	void RagdollSystem::on_ragdoll_created(entt::registry& registry, entt::entity entity_handle)
	{
		ScopedSpinLock _lock{ sync };

		ragdolls_to_init.push_back(static_cast<Entity::Handle>(entity_handle));
	}

}
