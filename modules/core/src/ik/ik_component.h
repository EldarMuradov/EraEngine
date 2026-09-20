#pragma once 

#include "core_api.h"

#include "ecs/component.h"

#include "core/math.h"

#include <optional>

namespace era_engine
{
    namespace animation
    {
        class Skeleton;
    }

    struct ERA_CORE_API IKBoneConstraint
    {
        float swing_min_deg = -45.0f;
        float swing_max_deg = 45.0f;

        bool swing_locked = false;
    };

    struct ERA_CORE_API IKChain
    {
        std::vector<uint32> joint_ids;
        std::vector<float> bone_lengths; // size = joint_ids.size() - 1.
        std::vector<IKBoneConstraint> constraints;// size = joint_ids.size().

        vec3 target_world_position = vec3(0.0f);
        vec3 pole_vector_local = vec3(0.0f, 0.0f, 1.0f);
        float weight = 1.0f;
        int32 iterations = 10;
        float tolerance = 0.005f;
        bool enabled = true;

        std::vector<vec3> joint_world_positions; // size = joint_ids.size()

        vec3 pole_vector_world = vec3(0.0f, 0.0f, 1.0f);
    };

	class ERA_CORE_API IKComponent : public Component
	{
	public:
		IKComponent(ref<Entity::EcsData> _data);
		~IKComponent() override;

        std::vector<IKChain> chains;
        bool enabled = true;

        uint32 find_chain_by_root_name(const animation::Skeleton& skeleton, std::string_view root_name) const;

        bool add_chain_to_component(const std::vector<std::string>& joint_names,
            const std::vector<IKBoneConstraint>* constraints = nullptr);

		ERA_VIRTUAL_REFLECT(Component)
	};

    class ERA_CORE_API IKTargetComponent : public Component
    {
    public:
        IKTargetComponent(ref<Entity::EcsData> _data);
        ~IKTargetComponent() override;

        Entity::Handle owner_entity = Entity::NullHandle;
        uint32 chain_index = 0;
        float weight = 1.0f;

        ERA_VIRTUAL_REFLECT(Component)
    };

}