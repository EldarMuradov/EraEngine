#include "ik/full_body_ik_component.h"

#include "animation/skeleton.h"
#include "animation/skeleton_component.h"

#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;
		registration::class_<FullBodyIKComponent>("FullBodyIKComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("enabled", &FullBodyIKComponent::enabled)
			.property("draw_debug", &FullBodyIKComponent::draw_debug);

		registration::class_<FullBodyIKTargetComponent>("FullBodyIKTargetComponent")
			.constructor<ref<Entity::EcsData>>()
			.property("position_weight", &FullBodyIKTargetComponent::position_weight)
			.property("rotation_weight", &FullBodyIKTargetComponent::rotation_weight);
	}

	FullBodyIKComponent::FullBodyIKComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	FullBodyIKComponent::~FullBodyIKComponent()
	{
	}

	uint32 FullBodyIKComponent::add_effector(std::string_view joint_name)
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

		FullBodyIKEffector& effector = effectors.emplace_back();
		effector.joint_id = joint_id;

		return static_cast<uint32>(effectors.size() - 1);
	}

	uint32 FullBodyIKComponent::find_effector(std::string_view joint_name) const
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

	bool FullBodyIKComponent::set_root(std::string_view joint_name)
	{
		const uint32 joint_id = get_joint_index(joint_name);
		if (joint_id == INVALID_JOINT)
		{
			return false;
		}

		root_joint_id = joint_id;
		return true;
	}

	bool FullBodyIKComponent::set_joint_settings(std::string_view joint_name, const FullBodyIKJointSettings& new_joint_settings)
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

	uint32 FullBodyIKComponent::get_joint_index(std::string_view joint_name) const
	{
		const animation::SkeletonComponent* skeleton_component = get_entity().get_component<animation::SkeletonComponent>();
		if (skeleton_component == nullptr || skeleton_component->skeleton == nullptr)
		{
			return INVALID_JOINT;
		}
		return skeleton_component->skeleton->get_joint_index(joint_name);
	}

	FullBodyIKTargetComponent::FullBodyIKTargetComponent(ref<Entity::EcsData> _data)
		: Component(_data)
	{
	}

	FullBodyIKTargetComponent::~FullBodyIKTargetComponent()
	{
	}
}
