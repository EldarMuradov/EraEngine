#include "ik/ik_component.h"

#include "animation/skeleton.h"
#include "animation/skeleton_component.h"

#include "core/log.h"

#include "ecs/serialization/component_registry.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::enumeration<FBIKSolverType>("FBIKSolverType")
			(
				value("JACOBIAN", FBIKSolverType::JACOBIAN),
				value("FABRIK", FBIKSolverType::FABRIK)
			);

		registration::enumeration<IKLimitType>("IKLimitType")
			(
				value("FREE", IKLimitType::FREE),
				value("LIMITED", IKLimitType::LIMITED),
				value("LOCKED", IKLimitType::LOCKED)
			);

		registration::class_<FBIKJointSettings>("FBIKJointSettings")
			.property("angular_stiffness", &FBIKJointSettings::angular_stiffness)
			.property("linear_stiffness", &FBIKJointSettings::linear_stiffness)
			.property("angular_limit_types", &FBIKJointSettings::angular_limit_types)
			.property("angular_limits_deg", &FBIKJointSettings::angular_limits_deg)
			.property("frame_offset", &FBIKJointSettings::frame_offset);

		registration::class_<IKChain>("IKChain")
			.property("joint_ids", &IKChain::joint_ids)
			.property("constraints", &IKChain::constraints)
			.property("rotation_weight", &IKChain::rotation_weight)
			.property("pole_vector_local", &IKChain::pole_vector_local)
			.property("pole_weight", &IKChain::pole_weight)
			.property("soft_reach_distance", &IKChain::soft_reach_distance)
			.property("weight", &IKChain::weight)
			.property("iterations", &IKChain::iterations)
			.property("tolerance", &IKChain::tolerance)
			.property("enabled", &IKChain::enabled);

		registration::class_<IKComponent>("IKComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("solver_type", &IKComponent::solver_type)
			.property("chains", &IKComponent::chains)
			.property("enabled", &IKComponent::enabled);
		ComponentRegistry::register_component<IKComponent>();

		registration::class_<IKTargetComponent>("IKTargetComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("owner_entity", &IKTargetComponent::owner_entity)
			.property("chain_index", &IKTargetComponent::chain_index)
			.property("weight", &IKTargetComponent::weight);
		ComponentRegistry::register_component<IKTargetComponent>();
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

	bool IKChain::init(const animation::Skeleton* skeleton, FBIKSolverType solver_type)
	{
		solver.reset();

		if (skeleton == nullptr || joint_ids.size() < 2)
		{
			return false;
		}

		effectors.resize(1);
		effectors.front().joint_id = joint_ids.back();

		std::vector<FBIKJointSettings> joint_settings(skeleton->joints.size());
		for (size_t i = 0; i < joint_ids.size() && i < constraints.size(); ++i)
		{
			if (joint_ids[i] < joint_settings.size())
			{
				joint_settings[joint_ids[i]] = constraints[i];
			}
		}

		solver = create_full_body_ik_solver(solver_type);
		if (!solver->init(skeleton, joint_ids.front(), effectors, joint_settings))
		{
			solver.reset();
			return false;
		}
		return true;
	}

	bool IKChain::solve(animation::SkeletonPose& pose, const trs& world_transform)
	{
		if (solver == nullptr || !solver->is_initialized())
		{
			return false;
		}

		IKEffector& effector = effectors.front();
		effector.target_position = target_world_position;
		effector.target_rotation = target_world_rotation;
		effector.position_weight = 1.0f;
		effector.rotation_weight = rotation_weight;
		effector.pole_vector_local = pole_vector_local;
		effector.pole_weight = pole_weight;
		effector.soft_reach_distance = soft_reach_distance;

		FBIKSettings settings;
		settings.iterations = iterations;
		settings.tolerance = tolerance;
		settings.weight = weight;

		return solver->solve(pose, world_transform, effectors, settings);
	}

	IKComponent::IKComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	IKComponent::~IKComponent()
	{
	}

	bool IKComponent::init()
	{
		if (is_initialized())
		{
			return true;
		}

		const animation::SkeletonComponent* skeleton_component = get_entity().get_component_if_exists<animation::SkeletonComponent>();
		if (skeleton_component == nullptr || skeleton_component->skeleton == nullptr)
		{
			return false;
		}

		bool result = true;
		for (IKChain& chain : chains)
		{
			result &= chain.init(skeleton_component->skeleton.get(), solver_type);
		}
		return result;
	}

	void IKComponent::on_deserialized()
	{
		init();
	}

	bool IKComponent::reinit()
	{
		reset();
		return init();
	}

	void IKComponent::reset()
	{
		for (IKChain& chain : chains)
		{
			chain.solver.reset();
		}
	}

	bool IKComponent::is_initialized() const
	{
		for (const IKChain& chain : chains)
		{
			if (chain.solver == nullptr || !chain.solver->is_initialized())
			{
				return false;
			}
		}
		return !chains.empty();
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
		const std::vector<FBIKJointSettings>* constraints/* = nullptr*/)
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
