#include "ik/full_body_ik_jacobian_solver.h"
#include "ik/private/full_body_ik_utils.h"

namespace era_engine
{
    static constexpr float rotation_tolerance = 0.01f;

    void FBIKJacobianSolver::on_init(const std::vector<IKEffector>& effectors)
    {
        const size_t nodes_count = nodes.size();

        columns.clear();
        for (uint32 n = 0; n < (uint32)nodes_count; ++n)
        {
            const FBIKJointSettings& joint = node_settings[n];

            for (uint32 pass = 0; pass < 2; ++pass)
            {
                const bool linear = pass == 0;
                const vec3& stiffness = linear ? joint.linear_stiffness : joint.angular_stiffness;
                for (uint32 axis_index = 0; axis_index < 3; ++axis_index)
                {
                    const float scale = 1.0f - clamp01(stiffness.data[axis_index]);
                    if (scale <= 0.0f)
                    {
                        continue;
                    }

                    vec3 frame_axis = vec3(0.0f);
                    frame_axis.data[axis_index] = 1.0f;

                    Column& column = columns.emplace_back();
                    column.node = n;
                    column.frame_axis = normalize(joint.frame_offset * frame_axis);
                    column.scale = scale;
                    column.is_linear = linear;
                }
            }
        }

        chains.assign(effectors.size(), EffectorChain{});
        chain_fractions.assign(nodes_count, 0.0f);

        for (uint32 n = 0; n < (uint32)nodes_count; ++n)
        {
            const uint32 effector_index = nodes[n].effector;
            if (effector_index == INVALID_JOINT)
            {
                continue;
            }

            EffectorChain& chain = chains[effector_index];
            chain.node = n;
            chain.nodes.assign(nodes_count, 0);

            uint32 chain_size = 0;
            for (uint32 node = n; node != INVALID_JOINT; node = nodes[node].parent)
            {
                ++chain_size;
            }
            const uint32 depth = std::max(std::min(effectors[effector_index].depth, chain_size), 1u);

            uint32 node = n;
            for (uint32 i = 0; i < depth; ++i)
            {
                chain.nodes[node] = 1;
                chain_fractions[node] = std::max(chain_fractions[node], 1.0f - (float)(i + 1) / (float)depth);
                node = nodes[node].parent;
            }
        }
    }

    void FBIKJacobianSolver::solve_nodes(const std::vector<IKEffector>& effectors, const FBIKSettings& settings)
    {
        if (columns.empty())
        {
            return;
        }

        motion_strengths.resize(nodes.size());
        for (size_t n = 0; n < nodes.size(); ++n)
        {
            motion_strengths[n] = lerp(settings.min_motion_strength, settings.max_motion_strength, chain_fractions[n]);
        }

        forward_kinematics();
        solve_limbs(effectors);

        for (int32 iter = 0; iter < settings.iterations; ++iter)
        {
            if (!solve_step(effectors, settings))
            {
                break;
            }

            apply_step();
            apply_constraints();
            solve_limbs(effectors);

            ++last_iterations;
        }
    }

    bool FBIKJacobianSolver::solve_step(const std::vector<IKEffector>& effectors, const FBIKSettings& settings)
    {
        using namespace ik_utils;

        rows.clear();

        bool converged = true;
        for (uint32 n = 0; n < (uint32)nodes.size(); ++n)
        {
            const uint32 limb_index = node_limbs[n];
            if (position_effectors[n])
            {
                Row& row = rows.emplace_back();
                row.effector = nodes[n].effector;
                row.node = limb_index != INVALID_JOINT ? limbs[limb_index].upper : n;
                row.goal = limb_index != INVALID_JOINT ? get_limb_upper_goal(limbs[limb_index], effective_target_positions[n]) : effective_target_positions[n];

                converged &= length(row.goal - world_solved_positions[row.node]) <= settings.tolerance;
            }

            if (rotation_effectors[n] && limb_index == INVALID_JOINT)
            {
                Row& row = rows.emplace_back();
                row.effector = nodes[n].effector;
                row.node = n;
                row.is_rotation = true;

                converged &= length(rotation_vector(effective_target_rotations[n] * conjugate(world_solved_rotations[n]))) <= rotation_tolerance;
            }
        }

        if (rows.empty() || converged)
        {
            return false;
        }

        const uint32 cols_count = (uint32)columns.size();

        column_world_axes.resize(cols_count);
        for (uint32 c = 0; c < cols_count; ++c)
        {
            const Column& column = columns[c];
            column_world_axes[c] = get_parent_rotation(column.node) * (rest_local_rotations[column.node] * column.frame_axis);
        }

        // Partial derivatives of the effector positions and rotations by the joint motions.
        jacobian.resize(3 * (uint32)rows.size(), cols_count);
        target_vector.resize(3 * (uint32)rows.size());

        bool movable = false;
        for (uint32 r = 0; r < (uint32)rows.size(); ++r)
        {
            const Row& row = rows[r];
            const EffectorChain& chain = chains[row.effector];

            for (uint32 c = 0; c < cols_count; ++c)
            {
                const Column& column = columns[c];
                if (!chain.nodes[column.node] || node_limbs[column.node] != INVALID_JOINT)
                {
                    continue;
                }

                movable = true;

                const float strength = motion_strengths[column.node] * column.scale;
                const vec3& axis = column_world_axes[c];

                vec3 derivative = vec3(0.0f);
                if (row.is_rotation)
                {
                    derivative = column.is_linear ? vec3(0.0f) : axis * strength;
                }
                else
                {
                    derivative = column.is_linear
                                     ? axis * strength
                                     : cross(axis, world_solved_positions[row.node] - world_solved_positions[column.node]) * strength;
                }
                jacobian.set_column_vec3(3 * r, c, derivative);
            }

            vec3 target = vec3(0.0f);
            if (row.is_rotation)
            {
                target = rotation_vector(effective_target_rotations[row.node] * conjugate(world_solved_rotations[row.node]));
            }
            else
            {
                float chain_length = 0.0f;
                for (uint32 node = row.node; node != INVALID_JOINT && chain.nodes[node]; node = nodes[node].parent)
                {
                    const uint32 parent = nodes[node].parent;
                    if (parent != INVALID_JOINT && chain.nodes[parent])
                    {
                        chain_length += local_animation_offset_lengths[node];
                    }
                }
                const float max_length = chain_length * (settings.target_clamp + clamp01(effectors[row.effector].pull) * (1.0f - settings.target_clamp));

                target = row.goal - world_solved_positions[row.node];
                const float target_length = length(target);
                if (target_length > max_length && target_length > EPSILON)
                {
                    target *= max_length / target_length;
                }
            }
            target_vector.set_vec3(3 * r, target);
        }

        if (!movable)
        {
            return false;
        }

        // deltas = J^T * (J * J^T + damping^2 * I)^-1 * e.
        multiply_transposed(jacobian, jacobian, system_matrix);
        system_matrix.add_to_diagonal(settings.damping * settings.damping);

        if (!solve_cholesky(system_matrix, target_vector))
        {
            return false;
        }

        transpose_multiply(jacobian, target_vector, deltas);
        return true;
    }

    void FBIKJacobianSolver::apply_step()
    {
        size_t c = 0;
        for (uint32 n = 0; n < (uint32)nodes.size(); ++n)
        {
            const uint32 parent = nodes[n].parent;
            if (parent != INVALID_JOINT)
            {
                world_solved_positions[n] = world_solved_positions[parent] + world_solved_rotations[parent] * local_solved_offsets[n];
                world_solved_rotations[n] = normalize(world_solved_rotations[parent] * local_solved_rotations[n]);
            }

            vec3 delta_position = vec3(0.0f);
            quat delta_rotation = quat::identity;
            for (; c < columns.size() && columns[c].node == n; ++c)
            {
                const float delta = deltas[c] * columns[c].scale;
                if (columns[c].is_linear)
                {
                    delta_position += column_world_axes[c] * delta;
                }
                else if (std::abs(delta) > 1e-9f)
                {
                    delta_rotation = delta_rotation * quat(column_world_axes[c], delta);
                }
            }

            world_solved_positions[n] += delta_position;
            world_solved_rotations[n] = normalize(delta_rotation * world_solved_rotations[n]);

            update_local_transform(n);
        }
    }

    void FBIKJacobianSolver::apply_constraints()
    {
        for (uint32 n = 0; n < (uint32)nodes.size(); ++n)
        {
            local_solved_rotations[n] = constrain_local_rotation(n, local_solved_rotations[n], false);
        }
        forward_kinematics();
    }

    void FBIKJacobianSolver::solve_limbs(const std::vector<IKEffector>& effectors)
    {
        for (const Limb& limb : limbs)
        {
            solve_limb(limb, effectors[nodes[limb.end].effector]);
            local_solved_rotations[limb.upper] = constrain_local_rotation(limb.upper, local_solved_rotations[limb.upper], true);
            local_solved_rotations[limb.middle] = constrain_local_rotation(limb.middle, local_solved_rotations[limb.middle], true);
        }

        if (!limbs.empty())
        {
            forward_kinematics();
        }
    }
}
