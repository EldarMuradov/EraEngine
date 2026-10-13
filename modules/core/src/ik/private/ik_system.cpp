#include "ik/ik_system.h"
#include "ik/ik_component.h"
#include "ik/full_body_ik_component.h"

#include "animation/skeleton.h"
#include "animation/skeleton_component.h"

#include "rendering/ecs/renderer_holder_root_component.h"
#include "rendering/debug_visualization.h"

#include "core/cpu_profiling.h"

#include "ecs/update_groups.h"
#include "ecs/base_components/transform_component.h"

#include <rttr/policy.h>
#include <rttr/registration>

namespace era_engine
{
	RTTR_REGISTRATION
	{
		using namespace rttr;

		registration::class_<IKSystem>("IKSystem")
			.constructor<World*>()(policy::ctor::as_raw_ptr, metadata("Tag", std::string("base")))
			.method("update", &IKSystem::update)
			(metadata("update_group", update_types::GAMEPLAY_NORMAL), 
				metadata("Before", std::vector<std::string>{"SkeletonSystem::update"}),
                metadata("After", std::vector<std::string>{"AnimationSystem::update"}))
			.method("debug_draw", &IKSystem::debug_draw)(metadata("update_group", update_types::RENDER));
	}

	IKSystem::IKSystem(World* _world)
		: System(_world)
	{
		renderer_holder_rc = world->add_root_component<RendererHolderRootComponent>();
		ASSERT(renderer_holder_rc != nullptr);
	}

	IKSystem::~IKSystem()
	{
	}

	void IKSystem::init()
	{
	}

	void IKSystem::update(float dt)
	{
        using namespace animation;

		ZoneScopedN("IKSystem::update");

        for (auto [entity_handle, target_component, transform_component] : world->group(components_group<IKTargetComponent, TransformComponent>).each())
        {
            IKComponent* owner_ik = world->get_entity(target_component.owner_entity).get_component<IKComponent>();
            if (!owner_ik)
            {
                continue;
            }

            if (target_component.chain_index >= owner_ik->chains.size())
            {
                continue;
            }

            auto& chain = owner_ik->chains[target_component.chain_index];

            const trs& target_transform = transform_component.get_world_transform();
            chain.target_world_position = target_transform.position;
            chain.target_world_rotation = target_transform.rotation;
            chain.weight = target_component.weight;
        }

        for (auto [entity_handle, target_component, transform_component] : world->group(components_group<FullBodyIKTargetComponent, TransformComponent>).each())
        {
            FullBodyIKComponent* owner_ik = world->get_entity(target_component.owner_entity).get_component_if_exists<FullBodyIKComponent>();
            if (owner_ik == nullptr)
            {
                continue;
            }

            if (target_component.effector_index >= owner_ik->effectors.size())
            {
                continue;
            }

            FullBodyIKEffector& effector = owner_ik->effectors[target_component.effector_index];

            const trs& target_transform = transform_component.get_world_transform();
            effector.target_position = target_transform.position;
            effector.target_rotation = target_transform.rotation;
            effector.position_weight = target_component.position_weight;
            effector.rotation_weight = target_component.rotation_weight;
        }

        for (auto [handle, full_body_ik_component, skeleton_component, transform_component]
            : world->group(components_group<FullBodyIKComponent,
                SkeletonComponent,
                TransformComponent>).each())
        {
            if (!full_body_ik_component.inited)
            {
                full_body_ik_component.solver.init(skeleton_component.skeleton.get(), full_body_ik_component.root_joint_id);
                full_body_ik_component.inited = true;
            }

            if (!full_body_ik_component.enabled || full_body_ik_component.settings.weight <= 0.0f)
            {
                continue;
            }

            SkeletonPose current_pose = skeleton_component.get_current_pose();

            if (!current_pose.is_valid())
            {
                continue;
            }

            const bool solved = full_body_ik_component.solver.solve(current_pose,
                transform_component.get_world_transform(),
                full_body_ik_component.effectors,
                full_body_ik_component.joint_settings,
                full_body_ik_component.settings);

            if (solved)
            {
                skeleton_component.apply_pose(current_pose);
            }
        }

        for (auto [handle, ik_component, skeleton_component, transform_component]
            : world->group(components_group<IKComponent,
                SkeletonComponent,
                TransformComponent>).each())
        {
            if (!ik_component.enabled)
            {
                continue;
            }

            SkeletonPose current_pose = skeleton_component.get_current_pose();

            if (!current_pose.is_valid())
            {
                continue;
            }

            const trs& world_transform = transform_component.get_world_transform();

            bool has_any_enabled = false;

            for (auto& chain : ik_component.chains)
            {
                if (!chain.enabled)
                {
                    continue;
                }
                if (chain.weight <= 0.0f)
                {
                    continue;
                }
                if (chain.solve(skeleton_component.skeleton.get(), current_pose, world_transform))
                {
                    has_any_enabled = true;
                }
            }

            if (has_any_enabled)
            {
                skeleton_component.apply_pose(current_pose);
            }
        }
	}

	void IKSystem::debug_draw(float dt)
	{
        for (auto [entity_handle, target_component, transform_component] : world->group(components_group<IKTargetComponent, TransformComponent>).each())
        {
            vec3 point = transform_component.get_world_transform().position;
            renderWireSphere(point, 0.05f, vec4(1.0f, 0.f, 1.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
        }

        for (auto [entity_handle, target_component, transform_component] : world->group(components_group<FullBodyIKTargetComponent, TransformComponent>).each())
        {
            vec3 point = transform_component.get_world_transform().position;
            renderWireSphere(point, 0.05f, vec4(1.0f, 0.f, 1.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
        }

        for (auto [entity_handle, full_body_ik_component] : world->view<FullBodyIKComponent>().each())
        {
            if (!full_body_ik_component.enabled || !full_body_ik_component.draw_debug)
            {
                continue;
            }

            const FullBodyIKSolver& solver = full_body_ik_component.solver;
            const std::vector<FullBodyIKSolver::Node>& nodes = solver.get_nodes();
            const std::vector<vec3>& positions = solver.get_solved_positions();

            if (positions.size() != nodes.size())
            {
                continue;
            }

            for (size_t i = 0; i < nodes.size(); ++i)
            {
                const vec4 color = nodes[i].effector != INVALID_JOINT ? vec4(0.0f, 1.0f, 1.0f, 1.0f) : vec4(1.0f, 1.0f, 0.0f, 1.0f);
                renderWireSphere(positions[i], 0.03f, color, renderer_holder_rc->ldrRenderPass);

                if (nodes[i].parent != INVALID_JOINT)
                {
                    renderLine(positions[nodes[i].parent], positions[i], vec4(1.0f, 1.0f, 0.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
                }
            }

            for (const FullBodyIKEffector& effector : full_body_ik_component.effectors)
            {
                if (effector.enabled && effector.position_weight > 0.0f)
                {
                    renderWireSphere(effector.target_position, 0.06f, vec4(0.0f, 1.f, 1.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
                }
            }
        }

        for (auto [entity_handle, ik_component] : world->view<IKComponent>().each())
        {
            for (const IKChain& chain : ik_component.chains)
            {
                if (!chain.enabled)
                {
                    continue;
                }

                for (const vec3& position : chain.solver.get_solved_positions())
                {
                    renderWireSphere(position, 0.05f, vec4(1.0f, 1.f, 0.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
                }
                renderWireSphere(chain.target_world_position, 0.06f, vec4(0.0f, 1.f, 1.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
            }
        }
	}
}
