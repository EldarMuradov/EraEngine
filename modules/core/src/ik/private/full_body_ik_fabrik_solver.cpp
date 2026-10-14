#include "ik/full_body_ik_fabrik_solver.h"

namespace era_engine
{
	static quat fit_rotation(quat rotation, const vec3* from, const vec3* to, size_t count, uint32 iterations = 8)
	{
		for (uint32 iter = 0; iter < iterations; ++iter)
		{
			vec3 omega = vec3(0.0f);
			float denominator = 0.0f;
			for (size_t i = 0; i < count; ++i)
			{
				const vec3 rotated = rotation * from[i];
				omega += cross(rotated, to[i]);
				denominator += dot(rotated, to[i]);
			}

			omega *= 1.0f / (std::abs(denominator) + 1e-9f);

			const float angle = length(omega);
			if (angle < 1e-7f)
			{
				break;
			}
			rotation = normalize(quat(omega / angle, angle) * rotation);
		}
		return rotation;
	}

	void FBIKFabrikSolver::solve_nodes(const std::vector<IKEffector>& effectors, const FBIKSettings& settings)
	{
		const size_t nodes_count = nodes.size();

		backward_positions.resize(nodes_count);
		position_goals.assign(nodes_count, 0);
		for (size_t n = nodes_count; n-- > 0;)
		{
			if (position_effectors[n])
			{
				position_goals[n] = 1;
			}
			if (position_goals[n] && nodes[n].parent != INVALID_JOINT)
			{
				position_goals[nodes[n].parent] = 1;
			}
		}

		last_error = compute_error();
		for (int32 iter = 0; iter < settings.iterations && last_error >= settings.tolerance; ++iter)
		{
			backward_pass();
			forward_pass(effectors);

			++last_iterations;
			last_error = compute_error();
		}
	}

	void FBIKFabrikSolver::backward_pass()
	{
		for (uint32 n = (uint32)nodes.size(); n-- > 0;)
		{
			const Node& node = nodes[n];

			if (!position_goals[n])
			{
				backward_positions[n] = world_solved_positions[n];
				continue;
			}

			if (position_effectors[n])
			{
				backward_positions[n] = effective_target_positions[n];
				continue;
			}

			const uint32 limb_index = node_limbs[n];
			if (limb_index != INVALID_JOINT)
			{
				const Limb& limb = limbs[limb_index];
				if (limb.upper == n)
				{
					backward_positions[n] = get_limb_upper_goal(limb, backward_positions[limb.end]);
				}
				else
				{
					backward_positions[n] = world_solved_positions[n];
				}
				continue;
			}

			// FABRIK step from every child towards the current position, a sub-base takes the centroid.
			vec3 sum = vec3(0.0f);
			uint32 count = 0;
			for (uint32 child : node.children)
			{
				if (!position_goals[child])
				{
					continue;
				}

				const float bone_length = local_animation_offset_lengths[child];
				const vec3 to_node = world_solved_positions[n] - backward_positions[child];
				const float distance = length(to_node);
				const vec3 direction = distance > EPSILON
					? to_node / distance
					: world_solved_rotations[n] * -local_animation_offsets[child] / std::max(bone_length, EPSILON);

				sum += backward_positions[child] + direction * bone_length;
				++count;
			}
			backward_positions[n] = sum / (float)count;
		}
	}

	void FBIKFabrikSolver::forward_pass(const std::vector<IKEffector>& effectors)
	{
		for (uint32 n = 0; n < (uint32)nodes.size(); ++n)
		{
			const Node& node = nodes[n];
			const quat& parent_rotation = get_parent_rotation(n);

			if (node.parent == INVALID_JOINT)
			{
				const vec3& linear_stiffness = node_settings[n].linear_stiffness;
				const float mobility = 1.0f - clamp01(std::min(linear_stiffness.x, std::min(linear_stiffness.y, linear_stiffness.z)));
				world_solved_positions[n] = position_effectors[n]
					? effective_target_positions[n]
					: lerp(world_animation_positions[n], backward_positions[n], mobility);
			}
			else
			{
				world_solved_positions[n] = world_solved_positions[node.parent] + world_solved_rotations[node.parent] * local_solved_offsets[n];
			}

			const uint32 limb_index = node_limbs[n];
			if (limb_index != INVALID_JOINT)
			{
				const Limb& limb = limbs[limb_index];
				if (limb.upper == n)
				{
					solve_limb(limb, effectors[nodes[limb.end].effector]);
				}

				if (limb.end != n)
				{
					local_solved_rotations[n] = constrain_local_rotation(n, local_solved_rotations[n], true);
				}

				world_solved_rotations[n] = normalize(parent_rotation * local_solved_rotations[n]);
				continue;
			}

			// The minimal rotation turning the bones to the children positions of the backward pass.
			const quat prior = normalize(parent_rotation * local_solved_rotations[n]);
			quat fitted = prior;

			fit_from.clear();
			fit_to.clear();
			for (uint32 child : node.children)
			{
				if (position_goals[child] && local_animation_offset_lengths[child] > EPSILON)
				{
					fit_from.push_back(local_solved_offsets[child]);
					fit_to.push_back(backward_positions[child] - world_solved_positions[n]);
				}
			}

			if (fit_from.size() == 1)
			{
				if (squared_length(fit_to[0]) > EPSILON * EPSILON)
				{
					fitted = normalize(rotate_from_to(prior * fit_from[0], fit_to[0]) * prior);
				}
			}
			else if (fit_from.size() > 1)
			{
				fitted = fit_rotation(prior, fit_from.data(), fit_to.data(), fit_from.size());
			}

			local_solved_rotations[n] = constrain_local_rotation(n, normalize(conjugate(parent_rotation) * fitted), true);
			world_solved_rotations[n] = normalize(parent_rotation * local_solved_rotations[n]);
		}
	}
}
