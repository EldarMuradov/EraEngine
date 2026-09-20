#include "ik/ik_component.h"

#include "animation/skeleton.h"
#include "animation/skeleton_component.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<IKComponent>("IKComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("enabled", &IKComponent::enabled);

		registration::class_<IKTargetComponent>("IKTargetComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("weight", &IKTargetComponent::weight);
	}

		static std::vector<trs> compute_default_object_pose(const animation::Skeleton& skeleton)
	{
		const size_t joints_nb = skeleton.joints.size();
		std::vector<trs> object_transforms(joints_nb);

		for (size_t i = 0; i < joints_nb; ++i)
		{
			const trs& local_transform = skeleton.default_local_transforms[i].get_transform();
			uint32 parent_id = skeleton.joints[i].parent_id;

			if (parent_id == INVALID_JOINT || parent_id == i)
			{
				object_transforms[i] = local_transform;
			}
			else
			{
				const trs& parent_object_transform = object_transforms[parent_id];
				object_transforms[i] = parent_object_transform * local_transform;
			}
		}

		return object_transforms;
	}

	static void bake_chain_from_default_pose(const animation::Skeleton& skeleton, IKChain& chain)
	{
        const size_t nb = chain.joint_ids.size();
        if (nb < 2)
        {
            return;
        }

        const std::vector<trs> object_transforms = compute_default_object_pose(skeleton);

        chain.bone_lengths.assign(nb - 1, 0.0f);
        chain.constraints.resize(nb);

        for (size_t i = 0; i + 1 < nb; ++i)
        {
            const uint32 a = chain.joint_ids[i];
            const uint32 b = chain.joint_ids[i + 1];
            chain.bone_lengths[i] =
                length(object_transforms[b].position - object_transforms[a].position);
        }

        for (size_t i = 0; i < nb; ++i)
        {
            IKBoneConstraint& c = chain.constraints[i];
            c.swing_locked = fuzzy_equals(c.swing_min_deg, 0.0f)
                && fuzzy_equals(c.swing_max_deg, 0.0f);
        }

        chain.joint_world_positions.assign(nb, vec3(0.0f));
	}

	IKComponent::IKComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	IKComponent::~IKComponent()
	{
	}

	uint32 IKComponent::find_chain_by_root_name(const animation::Skeleton& skeleton, std::string_view root_name) const
	{
		uint32 target_id = skeleton.get_joint_index(root_name);
		if (target_id == INVALID_JOINT)
		{
			return target_id;
		}

		for (size_t i = 0; i < chains.size(); ++i)
		{
			if (!chains[i].joint_ids.empty() &&
				chains[i].joint_ids.front() == target_id)
			{
				return static_cast<uint32>(i);
			}
		}
		return INVALID_JOINT;
	}

	IKTargetComponent::IKTargetComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	IKTargetComponent::~IKTargetComponent()
	{
	}

	bool IKComponent::add_chain_to_component(const std::vector<std::string>& joint_names,
		const std::vector<IKBoneConstraint>* constraints/* = nullptr*/)
	{
		IKChain chain;
		chain.joint_ids.reserve(joint_names.size());

		const ref<animation::Skeleton>& skeleton = get_entity().get_component<animation::SkeletonComponent>()->skeleton;

		for (const auto& nm : joint_names)
		{
			uint32 id = skeleton->get_joint_index(nm);
			if (id == INVALID_JOINT)
			{
				return false;
			}
			chain.joint_ids.push_back(id);
		}

		if (chain.joint_ids.size() < 2)
		{
			return false;
		}

		if (constraints != nullptr)
		{
			chain.constraints = *constraints;
			chain.constraints.resize(chain.joint_ids.size());
		}
		else
		{
			chain.constraints.resize(chain.joint_ids.size());
		}

		bake_chain_from_default_pose(*skeleton.get(), chain);

		chains.push_back(std::move(chain));

		return true;
	}
}