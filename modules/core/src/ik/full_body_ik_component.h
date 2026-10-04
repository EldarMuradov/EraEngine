#pragma once

#include "core_api.h"

#include "ecs/component.h"

#include "ik/full_body_ik_solver.h"

namespace era_engine
{
	class ERA_CORE_API FullBodyIKComponent : public Component
	{
	public:
		FullBodyIKComponent(ref<Entity::EcsData> _data);
		~FullBodyIKComponent() override;

		uint32 add_effector(std::string_view joint_name);
		uint32 find_effector(std::string_view joint_name) const;

		bool set_root(std::string_view joint_name);
		bool set_joint_settings(std::string_view joint_name, const FullBodyIKJointSettings& joint_settings);

		std::vector<FullBodyIKEffector> effectors;
		std::vector<FullBodyIKJointSettings> joint_settings; // Indexed by joint id.

		FullBodyIKSettings settings;

		// INVALID_JOINT -> the lowest joint of the active effectors.
		uint32 root_joint_id = INVALID_JOINT;

		bool enabled = true;
		bool inited = false;
		bool draw_debug = true;

		FullBodyIKSolver solver;

		ERA_VIRTUAL_REFLECT(Component)

	private:
		uint32 get_joint_index(std::string_view joint_name) const;
	};

	class ERA_CORE_API FullBodyIKTargetComponent : public Component
	{
	public:
		FullBodyIKTargetComponent(ref<Entity::EcsData> _data);
		~FullBodyIKTargetComponent() override;

		Entity::Handle owner_entity = Entity::NullHandle;
		uint32 effector_index = 0;

		float position_weight = 1.0f;
		float rotation_weight = 0.0f;

		ERA_VIRTUAL_REFLECT(Component)
	};
}
