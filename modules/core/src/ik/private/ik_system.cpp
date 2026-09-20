#include "ik/ik_system.h"
#include "ik/ik_component.h"

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

        for (auto [entity_handle, target_component, transform_component] : world->group<IKTargetComponent, TransformComponent>().each())
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

            chain.target_world_position = transform_component.get_world_transform().position;
            chain.weight = target_component.weight;
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
                if (chain.joint_ids.size() < 2)
                {
                    continue;
                }
                if (chain.bone_lengths.size() + 1 != chain.joint_ids.size())
                {
                    continue;
                }

                has_any_enabled = true;
                solve_chain(chain, *skeleton_component.skeleton.get(), current_pose, world_transform);
            }

            if (has_any_enabled)
            {
                skeleton_component.apply_pose(current_pose);
            }
        }
	}

	void IKSystem::debug_draw(float dt)
	{
        for (auto [entity_handle, target_component, transform_component] : world->group<IKTargetComponent, TransformComponent>().each())
        {
            vec3 point = transform_component.get_world_transform().position;
            renderWireSphere(point, 0.05f, vec4(1.0f, 0.f, 1.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
        }

        for (auto [entity_handle, ik_component] : world->view<IKComponent>().each())
        {
            for (const IKChain& chain : ik_component.chains)
            {
                for (const vec3& position : chain.joint_world_positions)
                {
                    renderWireSphere(position, 0.05f, vec4(1.0f, 1.f, 0.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
                }
                renderWireSphere(chain.target_world_position, 0.06f, vec4(0.0f, 1.f, 1.0f, 1.0f), renderer_holder_rc->ldrRenderPass);
            }
        }
	}

    void IKSystem::solve_chain(IKChain& chain, const animation::Skeleton& skeleton, animation::SkeletonPose& pose, const trs& world_transform)
    {
        using namespace animation;

        if (chain.joint_ids.size() < 2)
        {
            return;
        }
        if (chain.bone_lengths.size() + 1 != chain.joint_ids.size())
        {
            return;
        }
        if (chain.constraints.size() != chain.joint_ids.size())
        {
            return;
        }

        solve_fabrik(chain, skeleton, pose, world_transform);
        //apply_constraints(chain, skeleton, pose, world_transform);
        write_back(chain, skeleton, pose, world_transform);
    }

    void IKSystem::solve_fabrik(IKChain& chain,
        const animation::Skeleton& skeleton,
        const animation::SkeletonPose& pose,
        const trs& world_transform)
    {
        const size_t n = chain.joint_ids.size();
        chain.joint_world_positions.resize(n);

        std::vector<trs> full_world(skeleton.joints.size());
        for (size_t i = 0; i < skeleton.joints.size(); ++i)
        {
            const trs& local = pose.get_joint_transform((uint32)i).get_transform();
            const uint32 pid = skeleton.joints[i].parent_id;

            if (pid == INVALID_JOINT || pid == i)
            {
                full_world[i] = world_transform * local;
            }
            else
            {
                full_world[i] = full_world[pid] * local;
            }
        }

        for (size_t i = 0; i < n; ++i)
        {
            chain.joint_world_positions[i] = full_world[chain.joint_ids[i]].position;
        }

        const vec3 root_pos = chain.joint_world_positions.front();
        const vec3 target = chain.target_world_position;

        float total_len = 0.0f;
        for (float L : chain.bone_lengths)
        {
            total_len += L;
        }

        const float dist = length(target - root_pos);

        if (dist > total_len)
        {
            const vec3 dir = (dist > EPSILON) ? (target - root_pos) / dist : vec3::up;
            for (size_t i = 1; i < n; ++i)
            {
                chain.joint_world_positions[i] = chain.joint_world_positions[i - 1] + dir * chain.bone_lengths[i - 1];
            }
            return;
        }

        for (int32 iter = 0; iter < chain.iterations; ++iter)
        {
            chain.joint_world_positions[n - 1] = target;
            for (int i = (int)n - 2; i >= 0; --i)
            {
                const vec3  d = chain.joint_world_positions[i] - chain.joint_world_positions[i + 1];
                const float len = length(d);
                const vec3  dir = (len > EPSILON) ? d / len : vec3::up;
                chain.joint_world_positions[i] = chain.joint_world_positions[i + 1]
                    + dir * chain.bone_lengths[i];
            }

            chain.joint_world_positions[0] = root_pos;
            for (size_t i = 1; i < n; ++i)
            {
                const vec3  d = chain.joint_world_positions[i] - chain.joint_world_positions[i - 1];
                const float len = length(d);
                const vec3  dir = (len > EPSILON) ? d / len : vec3::up;
                chain.joint_world_positions[i] = chain.joint_world_positions[i - 1]
                    + dir * chain.bone_lengths[i - 1];
            }

            if (n >= 3)
            {
                apply_pole_vector(chain, (uint32)(n - 2), world_transform);
            }

            if (length(chain.joint_world_positions[n - 1] - target) < chain.tolerance)
            {
                break;
            }
        }
    }

    void IKSystem::apply_pole_vector(IKChain& chain, 
        uint32 joint_idx,
        const trs& world_transform)
    {
        const uint32 n = (uint32)chain.joint_world_positions.size();
        if (joint_idx == 0 || joint_idx >= n - 1)
        {
            return;
        }

        vec3& joint = chain.joint_world_positions[joint_idx];
        const vec3& prev = chain.joint_world_positions[joint_idx - 1];
        const vec3& next = chain.joint_world_positions[joint_idx + 1];

        vec3 axis = next - prev;
        const float axis_len = length(axis);
        if (axis_len < EPSILON)
        {
            return;
        }
        axis /= axis_len;

        chain.pole_vector_world = world_transform.rotation * chain.pole_vector_local;

        vec3 pole = chain.pole_vector_world;
        pole -= axis * dot(pole, axis);
        if (dot(pole, pole) < EPSILON)
        {
            return;
        }
        pole = normalize(pole);

        vec3 to_joint = joint - prev;
        to_joint -= axis * dot(to_joint, axis);
        const float radius = length(to_joint);
        if (radius < EPSILON)
        {
            return;
        }

        const float along = dot(joint - prev, axis);
        joint = prev + axis * along + pole * radius;
    }

    void IKSystem::write_back(IKChain& chain, 
        const animation::Skeleton& skeleton, 
        animation::SkeletonPose& pose, 
        const trs& world_transform)
    {
        using namespace animation;

        const size_t n = chain.joint_ids.size();
        if (n < 2)
        {
            return;
        }

        std::vector<vec3> solved_object(n, vec3(0.0f));
        const quat conj_world_rot = conjugate(world_transform.rotation);
        for (size_t i = 0; i < n; ++i)
        {
            solved_object[i] = conj_world_rot * (chain.joint_world_positions[i] - world_transform.position);
        }

        std::vector<trs> default_object_transforms(skeleton.joints.size());
        for (size_t j = 0; j < skeleton.joints.size(); ++j)
        {
            const trs& local = skeleton.default_local_transforms[j].get_transform();
            const uint32 pid = skeleton.joints[j].parent_id;

            if (pid == INVALID_JOINT || pid == j)
            {
                default_object_transforms[j] = local;
            }
            else
            {
                default_object_transforms[j] = default_object_transforms[pid] * local;
            }
        }

        for (size_t i = 0; i + 1 < n; ++i)
        {
            const uint32 index_from = chain.joint_ids[i];
            const uint32 index_to = chain.joint_ids[i + 1];

            std::vector<trs> object_transforms(skeleton.joints.size());
            for (size_t j = 0; j < skeleton.joints.size(); ++j)
            {
                const trs& local = pose.get_joint_transform((uint32)j).get_transform();
                const uint32 pid = skeleton.joints[j].parent_id;

                if (pid == INVALID_JOINT || pid == j)
                {
                    object_transforms[j] = local;
                }
                else
                {
                    object_transforms[j] = object_transforms[pid] * local;
                }
            }

            const trs& from_obj = object_transforms[index_from];
            const trs& to_obj = object_transforms[index_to];

            const vec3 from_object_space = from_obj.position;
            const vec3 to_old_object_space = to_obj.position;
            const vec3 to_new_object_space = solved_object[i + 1];

            const quat conj_from_rot = conjugate(from_obj.rotation);

            const vec3 d_cur = conj_from_rot * (to_old_object_space - from_obj.position);
            const vec3 d_new = conj_from_rot * (to_new_object_space - from_obj.position);

            const float len_cur = length(d_cur);
            const float len_new = length(d_new);
            if (len_cur < EPSILON || len_new < EPSILON)
            {
                continue;
            }

            const vec3 dir_current = d_cur / len_cur;
            vec3 dir_target = d_new / len_new;

            const trs& from_default = default_object_transforms[index_from];
            const trs& to_default = default_object_transforms[index_to];

            const quat conj_from_default_rot = conjugate(from_default.rotation);
            const vec3 d_rest = conj_from_default_rot * (to_default.position - from_default.position);

            const float len_rest = length(d_rest);
            if (len_rest < EPSILON)
            {
                continue;
            }
            const vec3 rest_dir = d_rest / len_rest;

            const IKBoneConstraint& c = chain.constraints[i];

            if (!c.swing_locked)
            {
                const float cos_swing = clamp(dot(rest_dir, dir_target), -1.0f, 1.0f);
                const float swing_angle = std::acos(cos_swing);

                const float swing_min_rad = deg2rad(c.swing_min_deg);
                const float swing_max_rad = deg2rad(c.swing_max_deg);
                const float effective_min = std::max(0.0f, swing_min_rad);
                const float effective_max = std::max(effective_min, swing_max_rad);

                const float swing_clamped = clamp(swing_angle, effective_min, effective_max);

                if (std::abs(swing_clamped - swing_angle) > 1e-5f)
                {
                    vec3 axis = cross(rest_dir, dir_target);
                    const float axis_len = length(axis);
                    if (axis_len > EPSILON)
                    {
                        axis /= axis_len;
                        const quat q(axis, swing_clamped);
                        dir_target = normalize(q * rest_dir);
                    }
                }
            }

            const quat delta = rotate_from_to(dir_current, dir_target);
            const quat current_local = pose.get_joint_rotation(index_from);
            quat final_rotation = normalize(current_local * delta);

            if (chain.weight < 1.0f)
            {
                final_rotation = normalize(slerp(current_local, final_rotation, chain.weight));
            }

            pose.set_joint_rotation(final_rotation, index_from);
        }
    }
}