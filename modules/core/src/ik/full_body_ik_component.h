#pragma once

#include "core_api.h"

#include "ecs/component.h"

#include "ik/full_body_ik_solver.h"

namespace era_engine
{
    class ERA_CORE_API FBIKComponent : public Component
    {
    public:
        FBIKComponent(ref<Entity::EcsData> _data);
        ~FBIKComponent() override;

        uint32 add_effector(std::string_view joint_name);
        uint32 find_effector(std::string_view joint_name) const;

        bool set_root(std::string_view joint_name);
        bool set_joint_settings(std::string_view joint_name, const FBIKJointSettings& joint_settings);

        bool init();

        bool reinit();
        void reset();
        bool is_initialized() const;

        void on_deserialized() override;

        FBIKSolverType solver_type = FBIKSolverType::FABRIK;

        std::vector<IKEffector> effectors;
        std::vector<FBIKJointSettings> joint_settings; // Indexed by joint id.

        FBIKSettings settings;

        // INVALID_JOINT -> the lowest common ancestor of the effectors.
        uint32 root_joint_id = INVALID_JOINT;

        bool enabled = true;
        bool draw_debug = true;

        ref<FBIKSolver> solver;

        ERA_VIRTUAL_REFLECT(Component)

    private:
        uint32 get_joint_index(std::string_view joint_name) const;
    };

    class ERA_CORE_API FBIKTargetComponent : public Component
    {
    public:
        FBIKTargetComponent(ref<Entity::EcsData> _data);
        ~FBIKTargetComponent() override;

        Entity::Handle owner_entity = Entity::null_handle;
        uint32 effector_index = 0;

        float position_weight = 1.0f;
        float rotation_weight = 0.0f;

        ERA_VIRTUAL_REFLECT(Component)
    };
}
