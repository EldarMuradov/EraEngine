#pragma once

#include "core_api.h"

#include "ecs/component.h"

#include "core/math.h"

#include "ik/full_body_ik_solver.h"

namespace era_engine
{
    namespace animation
    {
        class Skeleton;
        class SkeletonPose;
    }

    struct ERA_CORE_API IKChain
    {
        std::vector<uint32> joint_ids;
        std::vector<FBIKJointSettings> constraints; // size = joint_ids.size().

        vec3 target_world_position = vec3(0.0f);
        quat target_world_rotation = quat::identity;
        float rotation_weight = 0.0f;

        vec3 pole_vector_local = vec3(0.0f, 0.0f, 1.0f);
        float pole_weight = 1.0f;

        float soft_reach_distance = 0.02f;

        float weight = 1.0f;
        int32 iterations = 10;
        float tolerance = 0.005f;
        bool enabled = true;

        bool init(const animation::Skeleton* skeleton, FBIKSolverType solver_type);
        bool solve(animation::SkeletonPose& pose, const trs& world_transform);

        ref<FBIKSolver> solver;

    private:
        std::vector<IKEffector> effectors;
    };

    class ERA_CORE_API IKComponent : public Component
    {
    public:
        IKComponent(ref<Entity::EcsData> _data);
        ~IKComponent() override;

        bool init();

        bool reinit();
        void reset();
        bool is_initialized() const;

        FBIKSolverType solver_type = FBIKSolverType::FABRIK;

        std::vector<IKChain> chains;
        bool enabled = true;

        uint32 find_chain_by_root_name(const animation::Skeleton& skeleton, std::string_view root_name) const;

        bool add_chain_to_component(const std::vector<std::string>& joint_names,
            const std::vector<FBIKJointSettings>* constraints = nullptr);

        void on_deserialized() override;

        ERA_VIRTUAL_REFLECT(Component)
    };

    class ERA_CORE_API IKTargetComponent : public Component
    {
    public:
        IKTargetComponent(ref<Entity::EcsData> _data);
        ~IKTargetComponent() override;

        Entity::Handle owner_entity = Entity::null_handle;
        uint32 chain_index = 0;
        float weight = 1.0f;

        ERA_VIRTUAL_REFLECT(Component)
    };
}
