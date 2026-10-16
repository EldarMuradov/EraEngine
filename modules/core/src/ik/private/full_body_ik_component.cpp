#include "ik/full_body_ik_component.h"

#include "animation/skeleton.h"
#include "animation/skeleton_component.h"

#include "ecs/serialization/component_registry.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<IKEffector>("IKEffector")
			.property("joint_id", &IKEffector::joint_id)
			.property("position_weight", &IKEffector::position_weight)
			.property("rotation_weight", &IKEffector::rotation_weight)
			.property("solve_as_limb", &IKEffector::solve_as_limb)
			.property("pole_vector_local", &IKEffector::pole_vector_local)
			.property("pole_weight", &IKEffector::pole_weight)
			.property("soft_reach_distance", &IKEffector::soft_reach_distance)
			.property("min_reach_distance", &IKEffector::min_reach_distance)
			.property("enabled", &IKEffector::enabled)
			.property("depth", &IKEffector::depth)
			.property("pull", &IKEffector::pull);

		registration::class_<FBIKSettings>("FBIKSettings")
			.property("iterations", &FBIKSettings::iterations)
			.property("tolerance", &FBIKSettings::tolerance)
			.property("weight", &FBIKSettings::weight)
			.property("damping", &FBIKSettings::damping)
			.property("max_motion_strength", &FBIKSettings::max_motion_strength)
			.property("min_motion_strength", &FBIKSettings::min_motion_strength)
			.property("target_clamp", &FBIKSettings::target_clamp);

		registration::class_<FBIKComponent>("FBIKComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("solver_type", &FBIKComponent::solver_type)
			.property("effectors", &FBIKComponent::effectors)
			.property("joint_settings", &FBIKComponent::joint_settings)
			.property("settings", &FBIKComponent::settings)
			.property("root_joint_id", &FBIKComponent::root_joint_id)
			.property("enabled", &FBIKComponent::enabled)
			.property("draw_debug", &FBIKComponent::draw_debug);
		ComponentRegistry::register_component<FBIKComponent>();

		registration::class_<FBIKTargetComponent>("FBIKTargetComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("owner_entity", &FBIKTargetComponent::owner_entity)
			.property("effector_index", &FBIKTargetComponent::effector_index)
			.property("position_weight", &FBIKTargetComponent::position_weight)
			.property("rotation_weight", &FBIKTargetComponent::rotation_weight);
		ComponentRegistry::register_component<FBIKTargetComponent>();
	}

	FBIKComponent::FBIKComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	FBIKComponent::~FBIKComponent()
	{
	}

	uint32 FBIKComponent::add_effector(std::string_view joint_name)
	{
		const uint32 joint_id = get_joint_index(joint_name);
		if (joint_id == INVALID_JOINT)
		{
			return INVALID_JOINT;
		}

		const uint32 existing = find_effector(joint_name);
		if (existing != INVALID_JOINT)
		{
			return existing;
		}

		IKEffector& effector = effectors.emplace_back();
		effector.joint_id = joint_id;

		return static_cast<uint32>(effectors.size() - 1);
	}

	uint32 FBIKComponent::find_effector(std::string_view joint_name) const
	{
		const uint32 joint_id = get_joint_index(joint_name);
		if (joint_id == INVALID_JOINT)
		{
			return INVALID_JOINT;
		}

		for (size_t i = 0; i < effectors.size(); ++i)
		{
			if (effectors[i].joint_id == joint_id)
			{
				return static_cast<uint32>(i);
			}
		}
		return INVALID_JOINT;
	}

	bool FBIKComponent::set_root(std::string_view joint_name)
	{
		const uint32 joint_id = get_joint_index(joint_name);
		if (joint_id == INVALID_JOINT)
		{
			return false;
		}

		root_joint_id = joint_id;
		return true;
	}

	bool FBIKComponent::set_joint_settings(std::string_view joint_name, const FBIKJointSettings& new_joint_settings)
	{
		const uint32 joint_id = get_joint_index(joint_name);
		if (joint_id == INVALID_JOINT)
		{
			return false;
		}

		if (joint_settings.size() <= joint_id)
		{
			joint_settings.resize(joint_id + 1);
		}
		joint_settings[joint_id] = new_joint_settings;

		return true;
	}

	bool FBIKComponent::init()
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

		solver = create_full_body_ik_solver(solver_type);
		if (!solver->init(skeleton_component->skeleton.get(), root_joint_id, effectors, joint_settings))
		{
			solver.reset();
			return false;
		}
		return true;
	}

	void FBIKComponent::on_deserialized()
	{
		init();
	}

	bool FBIKComponent::reinit()
	{
		reset();
		return init();
	}

	void FBIKComponent::reset()
	{
		solver.reset();
	}

	bool FBIKComponent::is_initialized() const
	{
		return solver != nullptr && solver->is_initialized();
	}

	uint32 FBIKComponent::get_joint_index(std::string_view joint_name) const
	{
		const animation::SkeletonComponent* skeleton_component = get_entity().get_component<animation::SkeletonComponent>();
		if (skeleton_component == nullptr || skeleton_component->skeleton == nullptr)
		{
			return INVALID_JOINT;
		}
		return skeleton_component->skeleton->get_joint_index(joint_name);
	}

	FBIKTargetComponent::FBIKTargetComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	FBIKTargetComponent::~FBIKTargetComponent()
	{
	}
}
