#include "ik/ik_component.h"

#include "animation/skeleton.h"
#include "animation/skeleton_component.h"

#include "core/log.h"

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

	static bool is_ancestor_or_self(const animation::Skeleton& skeleton, uint32 ancestor_id, uint32 joint_id)
	{
		while (joint_id != INVALID_JOINT && joint_id < skeleton.joints.size())
		{
			if (joint_id == ancestor_id)
			{
				return true;
			}

			const uint32 parent_id = skeleton.joints[joint_id].parent_id;
			joint_id = parent_id == joint_id ? INVALID_JOINT : parent_id;
		}
		return false;
	}

	bool IKChain::solve(const animation::Skeleton* skeleton, animation::SkeletonPose& pose, const trs& world_transform)
	{
		if (skeleton == nullptr || joint_ids.size() < 2)
		{
			return false;
		}

		solver.init(skeleton, joint_ids.front());

		effectors.resize(1);

		FullBodyIKEffector& effector = effectors.front();
		effector.joint_id = joint_ids.back();
		effector.target_position = target_world_position;
		effector.target_rotation = target_world_rotation;
		effector.position_weight = 1.0f;
		effector.rotation_weight = rotation_weight;
		effector.pole_vector_local = pole_vector_local;
		effector.pole_weight = pole_weight;
		effector.soft_reach = soft_reach;

		joint_settings.assign(skeleton->joints.size(), FullBodyIKJointSettings{});
		for (size_t i = 0; i < joint_ids.size() && i < constraints.size(); ++i)
		{
			if (joint_ids[i] < joint_settings.size())
			{
				joint_settings[joint_ids[i]] = constraints[i];
			}
		}

		FullBodyIKSettings settings;
		settings.iterations = iterations;
		settings.tolerance = tolerance;
		settings.root_position_mobility = 0.0f;
		settings.root_rotation_mobility = 1.0f;
		settings.weight = weight;

		return solver.solve(pose, world_transform, effectors, joint_settings, settings);
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
		const std::vector<FullBodyIKJointSettings>* constraints/* = nullptr*/)
	{
		const animation::SkeletonComponent* skeleton_component = get_entity().get_component<animation::SkeletonComponent>();
		if (skeleton_component == nullptr || skeleton_component->skeleton == nullptr)
		{
			return false;
		}

		const animation::Skeleton& skeleton = *skeleton_component->skeleton.get();

		IKChain chain;
		chain.joint_ids.reserve(joint_names.size());

		for (const auto& nm : joint_names)
		{
			uint32 id = skeleton.get_joint_index(nm);
			if (id == INVALID_JOINT)
			{
				return false;
			}

			if (!chain.joint_ids.empty() && !is_ancestor_or_self(skeleton, chain.joint_ids.back(), id))
			{
				LOG_ERROR("IK> Joint %s is not a descendant of the previous chain joint.", nm.c_str());
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
		}
		chain.constraints.resize(chain.joint_ids.size());

		chains.push_back(std::move(chain));

		return true;
	}
}
