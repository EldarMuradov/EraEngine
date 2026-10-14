#pragma once

#include "core_api.h"

#include "ik/full_body_ik_solver.h"

namespace era_engine
{
	// Jacobian solver (DLS).
	class ERA_CORE_API FBIKJacobianSolver final : public FBIKSolver
	{
	protected:
		void on_init(const std::vector<IKEffector>& effectors) override;
		void solve_nodes(const std::vector<IKEffector>& effectors, const FBIKSettings& settings) override;

	private:
		struct Column
		{
			uint32 node = INVALID_JOINT;
			vec3 frame_axis = vec3(0.0f);
			float scale = 1.0f; // 1 - stiffness.
			bool is_linear = false;
		};

		struct EffectorChain
		{
			uint32 node = INVALID_JOINT;
			std::vector<uint8> nodes; // 1 for the nodes the effector moves.
		};

		bool solve_step(const std::vector<IKEffector>& effectors, const FBIKSettings& settings);
		void apply_step();
		void apply_constraints();
		void solve_limbs(const std::vector<IKEffector>& effectors);

		std::vector<Column> columns; // Nodes in topological order, the linear axes of a node first.
		std::vector<EffectorChain> chains; // Per effector.
		std::vector<float> chain_fractions; // Per node, [0, 1] from the chain root to the effector.

		std::vector<vec3> column_world_axes;
		std::vector<float> motion_strengths; // Per node.

		std::vector<uint32> row_effectors; // Per 3 rows, the effector index.
		std::vector<uint8> row_rotations; // Per 3 rows, 1 for a rotation target.
		std::vector<uint32> row_points; // Per 3 rows, the node the row moves.
		std::vector<vec3> row_goals; // Per 3 rows, the position goal of the node.

		std::vector<float> jacobian;
		std::vector<float> system_matrix;
		std::vector<float> target_vector;
		std::vector<float> system_vector;
		std::vector<float> deltas;
	};
}
